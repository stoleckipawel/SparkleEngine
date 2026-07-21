#include "PCH.h"

#include "Shaders/LoadedShaderPackage.h"

#include "Core/Public/Files/BinarySpanReader.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageContract.h"
#include "Shaders/ShaderRayTracingMetadataValidation.h"

#include <array>
#include <chrono>
#include <format>
#include <string>
#include <vector>

bool LoadedShaderPackage::IsRuntimeBinary(const CookedShaderBinaryRecord& record, CookedShaderBinaryFormat format) const noexcept
{
	return record.Format == format && ResolveString(record.CodegenTarget) == GetRuntimeShaderCodegenTarget(format);
}

const CookedShaderBinaryRecord* LoadedShaderPackage::FindRuntimeBinaryRecord(
    ShaderStage stage,
    CookedShaderBinaryFormat format) const noexcept
{
	for (const CookedShaderBinaryRecord& binaryRecord : m_binaryRecords)
	{
		if (binaryRecord.Stage == stage && IsRuntimeBinary(binaryRecord, format))
		{
			return &binaryRecord;
		}
	}

	return nullptr;
}

ShaderBytecode LoadedShaderPackage::GetBytecode(const CookedShaderBinaryRecord& record) const noexcept
{
	if (!ContainsBlobRef(record.Bytecode))
	{
		return {};
	}

	const std::uint8_t* bytecodeBegin = m_binaryBlob.data() + record.Bytecode.OffsetInBytes;
	return {bytecodeBegin, record.Bytecode.SizeInBytes};
}

bool LoadedShaderPackage::ValidateRayTracingLibraryMetadata(
    const RhiRayTracingCapabilities& capabilities,
    CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage) const
{
	if (m_header.PackageKind != CookedShaderPackageKind::RayTracingLibrary)
	{
		outErrorMessage.clear();
		return true;
	}

	if (!capabilities.SupportsRayTracing)
	{
		outErrorMessage = "Cooked ray tracing library requires pipeline ray tracing, but the active RHI backend reports it unsupported";
		return false;
	}

	if (m_rayTracingExports.empty())
	{
		outErrorMessage = "Cooked ray tracing library has no ray tracing export records";
		return false;
	}

	if (m_header.RayTracingMaxRecursionDepth == 0)
	{
		outErrorMessage = "Cooked ray tracing library has RayTracingMaxRecursionDepth=0";
		return false;
	}

	if (capabilities.MaxTraceRecursionDepth != 0 && m_header.RayTracingMaxRecursionDepth > capabilities.MaxTraceRecursionDepth)
	{
		outErrorMessage = std::format(
		    "Cooked ray tracing library recursion depth {} exceeds RHI limit {}",
		    m_header.RayTracingMaxRecursionDepth,
		    capabilities.MaxTraceRecursionDepth);
		return false;
	}

	if (capabilities.MaxRayPayloadSizeInBytes != 0 && m_header.RayTracingPayloadSizeInBytes > capabilities.MaxRayPayloadSizeInBytes)
	{
		outErrorMessage = std::format(
		    "Cooked ray tracing library payload size {} exceeds RHI limit {}",
		    m_header.RayTracingPayloadSizeInBytes,
		    capabilities.MaxRayPayloadSizeInBytes);
		return false;
	}

	if (capabilities.MaxRayAttributeSizeInBytes != 0 && m_header.RayTracingAttributeSizeInBytes > capabilities.MaxRayAttributeSizeInBytes)
	{
		outErrorMessage = std::format(
		    "Cooked ray tracing library attribute size {} exceeds RHI limit {}",
		    m_header.RayTracingAttributeSizeInBytes,
		    capabilities.MaxRayAttributeSizeInBytes);
		return false;
	}

	for (std::size_t exportIndex = 0; exportIndex < m_rayTracingExports.size(); ++exportIndex)
	{
		const CookedShaderRayTracingExportRecord& exportRecord = m_rayTracingExports[exportIndex];
		if (exportRecord.Kind == CookedShaderRayTracingExportKind::None)
		{
			outErrorMessage = std::format("Cooked ray tracing export {} has kind=None", exportIndex);
			return false;
		}
		if (ResolveString(exportRecord.ExportName).empty())
		{
			outErrorMessage = std::format("Cooked ray tracing export {} has an invalid ExportName string", exportIndex);
			return false;
		}
		if (ResolveString(exportRecord.EntryPoint).empty())
		{
			outErrorMessage = std::format("Cooked ray tracing export {} has an invalid EntryPoint string", exportIndex);
			return false;
		}
		if (exportRecord.BinaryRecordIndex >= m_binaryRecords.size())
		{
			outErrorMessage = std::format(
			    "Cooked ray tracing export {} references out-of-range binary record {}",
			    exportIndex,
			    exportRecord.BinaryRecordIndex);
			return false;
		}

		const CookedShaderBinaryRecord& binaryRecord = m_binaryRecords[exportRecord.BinaryRecordIndex];
		if (binaryRecord.ShaderBlobId == 0)
		{
			outErrorMessage = std::format("Cooked ray tracing export {} references a binary record with ShaderBlobId=0", exportIndex);
			return false;
		}
		if (!IsRuntimeBinary(binaryRecord, requiredBinaryFormat))
		{
			outErrorMessage = std::format(
			    "Cooked ray tracing export {} target '{}/{}' does not match RHI-required target '{}/{}'",
			    exportIndex,
			    CookedShaderBinaryFormatToString(binaryRecord.Format),
			    ResolveString(binaryRecord.CodegenTarget),
			    CookedShaderBinaryFormatToString(requiredBinaryFormat),
			    GetRuntimeShaderCodegenTarget(requiredBinaryFormat));
			return false;
		}
		if (!GetBytecode(binaryRecord).IsValid())
		{
			outErrorMessage = std::format("Cooked ray tracing export {} references an invalid bytecode blob", exportIndex);
			return false;
		}
	}

	for (std::size_t hitGroupIndex = 0; hitGroupIndex < m_rayTracingHitGroups.size(); ++hitGroupIndex)
	{
		const CookedShaderRayTracingHitGroupRecord& hitGroup = m_rayTracingHitGroups[hitGroupIndex];
		if (ResolveString(hitGroup.HitGroupName).empty())
		{
			outErrorMessage = std::format("Cooked ray tracing hit group {} has an invalid HitGroupName string", hitGroupIndex);
			return false;
		}
		if (hitGroup.ClosestHitExportIndex >= m_rayTracingExports.size())
		{
			outErrorMessage = std::format(
			    "Cooked ray tracing hit group {} references out-of-range closest-hit export {}",
			    hitGroupIndex,
			    hitGroup.ClosestHitExportIndex);
			return false;
		}
		if (hitGroup.AnyHitExportIndex != UINT32_MAX && hitGroup.AnyHitExportIndex >= m_rayTracingExports.size())
		{
			outErrorMessage = std::format(
			    "Cooked ray tracing hit group {} references out-of-range any-hit export {}",
			    hitGroupIndex,
			    hitGroup.AnyHitExportIndex);
			return false;
		}
		if (hitGroup.IntersectionExportIndex != UINT32_MAX && hitGroup.IntersectionExportIndex >= m_rayTracingExports.size())
		{
			outErrorMessage = std::format(
			    "Cooked ray tracing hit group {} references out-of-range intersection export {}",
			    hitGroupIndex,
			    hitGroup.IntersectionExportIndex);
			return false;
		}
		if (hitGroup.Type == CookedShaderRayTracingHitGroupType::ProceduralPrimitive && hitGroup.IntersectionExportIndex == UINT32_MAX)
		{
			outErrorMessage = std::format("Cooked procedural ray tracing hit group {} is missing an intersection export", hitGroupIndex);
			return false;
		}
	}

	for (std::size_t parameterIndex = 0; parameterIndex < m_rayTracingLocalParameters.size(); ++parameterIndex)
	{
		const CookedShaderRayTracingLocalParameterRecord& localParameter = m_rayTracingLocalParameters[parameterIndex];
		if (ResolveString(localParameter.Name).empty())
		{
			outErrorMessage = std::format("Cooked ray tracing local parameter {} has an invalid Name string", parameterIndex);
			return false;
		}
		if (localParameter.OwnerExportIndex >= m_rayTracingExports.size())
		{
			outErrorMessage = std::format(
			    "Cooked ray tracing local parameter {} references out-of-range owner export {}",
			    parameterIndex,
			    localParameter.OwnerExportIndex);
			return false;
		}
		if (localParameter.BindingRecordOffset + localParameter.BindingRecordCount > m_bindingRecords.size())
		{
			outErrorMessage = std::format("Cooked ray tracing local parameter {} binding range is out of bounds", parameterIndex);
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

std::string_view LoadedShaderPackage::ResolveString(CookedShaderStringRef ref) const noexcept
{
	if (!ContainsStringRef(ref))
	{
		return {};
	}

	const char* stringBegin = reinterpret_cast<const char*>(m_stringTable.data() + ref.OffsetInBytes);
	return std::string_view(stringBegin, ref.SizeInBytes);
}

bool LoadedShaderPackage::ContainsStringRef(CookedShaderStringRef ref) const noexcept
{
	return ref.OffsetInBytes <= m_stringTable.size() && ref.SizeInBytes <= m_stringTable.size() - ref.OffsetInBytes;
}

bool LoadedShaderPackage::ContainsBlobRef(CookedShaderBlobRef ref) const noexcept
{
	return ref.OffsetInBytes <= m_binaryBlob.size() && ref.SizeInBytes <= m_binaryBlob.size() - ref.OffsetInBytes;
}
