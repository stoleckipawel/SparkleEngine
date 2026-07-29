#include "PCH.h"

#include "Shaders/LoadedShaderPackage.h"

#include "Core/Public/Diagnostics/Error.h"
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

void LoadedShaderPackage::ValidateRayTracingLibraryMetadata(
    const RhiRayTracingCapabilities& capabilities,
    CookedShaderBinaryFormat runtimeBinaryFormat) const
{
	if (m_header.PackageKind != CookedShaderPackageKind::RayTracingLibrary)
	{
		return;
	}

	if (!capabilities.SupportsRayTracing)
	{
		throw Diagnostics::Error(
		    "Cooked ray tracing library uses pipeline ray tracing, but the active RHI backend reports it unsupported");
	}

	if (m_rayTracingExports.empty())
	{
		throw Diagnostics::Error("Cooked ray tracing library has no ray tracing export records");
	}

	if (m_header.RayTracingMaxRecursionDepth == 0)
	{
		throw Diagnostics::Error("Cooked ray tracing library has RayTracingMaxRecursionDepth=0");
	}

	if (capabilities.MaxTraceRecursionDepth != 0 && m_header.RayTracingMaxRecursionDepth > capabilities.MaxTraceRecursionDepth)
	{
		throw Diagnostics::Error(std::format(
		    "Cooked ray tracing library recursion depth {} exceeds RHI limit {}",
		    m_header.RayTracingMaxRecursionDepth,
		    capabilities.MaxTraceRecursionDepth));
	}

	if (capabilities.MaxRayPayloadSizeInBytes != 0 && m_header.RayTracingPayloadSizeInBytes > capabilities.MaxRayPayloadSizeInBytes)
	{
		throw Diagnostics::Error(std::format(
		    "Cooked ray tracing library payload size {} exceeds RHI limit {}",
		    m_header.RayTracingPayloadSizeInBytes,
		    capabilities.MaxRayPayloadSizeInBytes));
	}

	if (capabilities.MaxRayAttributeSizeInBytes != 0 && m_header.RayTracingAttributeSizeInBytes > capabilities.MaxRayAttributeSizeInBytes)
	{
		throw Diagnostics::Error(std::format(
		    "Cooked ray tracing library attribute size {} exceeds RHI limit {}",
		    m_header.RayTracingAttributeSizeInBytes,
		    capabilities.MaxRayAttributeSizeInBytes));
	}

	for (std::size_t exportIndex = 0; exportIndex < m_rayTracingExports.size(); ++exportIndex)
	{
		const CookedShaderRayTracingExportRecord& exportRecord = m_rayTracingExports[exportIndex];
		if (exportRecord.Kind == CookedShaderRayTracingExportKind::None)
		{
			throw Diagnostics::Error(std::format("Cooked ray tracing export {} has kind=None", exportIndex));
		}
		if (ResolveString(exportRecord.ExportName).empty())
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing export {} has an invalid ExportName string", exportIndex));
		}
		if (ResolveString(exportRecord.EntryPoint).empty())
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing export {} has an invalid EntryPoint string", exportIndex));
		}
		if (exportRecord.BinaryRecordIndex >= m_binaryRecords.size())
		{
			throw Diagnostics::Error(std::format(
			    "Cooked ray tracing export {} references out-of-range binary record {}",
			    exportIndex,
			    exportRecord.BinaryRecordIndex));
		}

		const CookedShaderBinaryRecord& binaryRecord = m_binaryRecords[exportRecord.BinaryRecordIndex];
		if (binaryRecord.ShaderBlobId == 0)
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing export {} references a binary record with ShaderBlobId=0", exportIndex));
		}
		if (!IsRuntimeBinary(binaryRecord, runtimeBinaryFormat))
		{
			throw Diagnostics::Error(std::format(
			    "Cooked ray tracing export {} target '{}/{}' does not match RHI runtime target '{}/{}'",
			    exportIndex,
			    CookedShaderBinaryFormatToString(binaryRecord.Format),
			    ResolveString(binaryRecord.CodegenTarget),
			    CookedShaderBinaryFormatToString(runtimeBinaryFormat),
			    GetRuntimeShaderCodegenTarget(runtimeBinaryFormat)));
		}
		if (!GetBytecode(binaryRecord).IsValid())
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing export {} references an invalid bytecode blob", exportIndex));
		}
	}

	for (std::size_t hitGroupIndex = 0; hitGroupIndex < m_rayTracingHitGroups.size(); ++hitGroupIndex)
	{
		const CookedShaderRayTracingHitGroupRecord& hitGroup = m_rayTracingHitGroups[hitGroupIndex];
		if (ResolveString(hitGroup.HitGroupName).empty())
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing hit group {} has an invalid HitGroupName string", hitGroupIndex));
		}
		if (hitGroup.ClosestHitExportIndex >= m_rayTracingExports.size())
		{
			throw Diagnostics::Error(std::format(
			    "Cooked ray tracing hit group {} references out-of-range closest-hit export {}",
			    hitGroupIndex,
			    hitGroup.ClosestHitExportIndex));
		}
		if (hitGroup.AnyHitExportIndex != UINT32_MAX && hitGroup.AnyHitExportIndex >= m_rayTracingExports.size())
		{
			throw Diagnostics::Error(std::format(
			    "Cooked ray tracing hit group {} references out-of-range any-hit export {}",
			    hitGroupIndex,
			    hitGroup.AnyHitExportIndex));
		}
		if (hitGroup.IntersectionExportIndex != UINT32_MAX && hitGroup.IntersectionExportIndex >= m_rayTracingExports.size())
		{
			throw Diagnostics::Error(std::format(
			    "Cooked ray tracing hit group {} references out-of-range intersection export {}",
			    hitGroupIndex,
			    hitGroup.IntersectionExportIndex));
		}
		if (hitGroup.Type == CookedShaderRayTracingHitGroupType::ProceduralPrimitive && hitGroup.IntersectionExportIndex == UINT32_MAX)
		{
			throw Diagnostics::Error(
			    std::format("Cooked procedural ray tracing hit group {} is missing an intersection export", hitGroupIndex));
		}
	}

	for (std::size_t parameterIndex = 0; parameterIndex < m_rayTracingLocalParameters.size(); ++parameterIndex)
	{
		const CookedShaderRayTracingLocalParameterRecord& localParameter = m_rayTracingLocalParameters[parameterIndex];
		if (ResolveString(localParameter.Name).empty())
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing local parameter {} has an invalid Name string", parameterIndex));
		}
		if (localParameter.OwnerExportIndex >= m_rayTracingExports.size())
		{
			throw Diagnostics::Error(std::format(
			    "Cooked ray tracing local parameter {} references out-of-range owner export {}",
			    parameterIndex,
			    localParameter.OwnerExportIndex));
		}
		if (localParameter.BindingRecordOffset + localParameter.BindingRecordCount > m_bindingRecords.size())
		{
			throw Diagnostics::Error(
			    std::format("Cooked ray tracing local parameter {} binding range is out of bounds", parameterIndex));
		}
	}
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
