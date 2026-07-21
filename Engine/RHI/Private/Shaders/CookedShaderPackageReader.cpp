#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

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

bool CookedShaderPackageCache::LoadPackageFromFile(
    const std::filesystem::path& path,
    LoadedShaderPackage& outPackage,
    std::string& outErrorMessage)
{
	std::vector<std::uint8_t> fileBytes;
	if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
	{
		return false;
	}

	Files::BinarySpanReader reader(fileBytes);
	if (!reader.ReadValue(outPackage.m_header, outErrorMessage))
	{
		return false;
	}

	if (!outPackage.m_header.Matches(kCookedShaderPackageMagic, kCookedShaderPackageVersion))
	{
		if (outPackage.m_header.Magic == kCookedShaderPackageMagic)
		{
			outErrorMessage = std::format(
			    "Cooked shader package '{}' is version {}. Recook to current version {}.",
			    path.string(),
			    outPackage.m_header.Version,
			    kCookedShaderPackageVersion);
			return false;
		}

		outErrorMessage = std::format(
		    "Invalid cooked shader package header in '{}': expected magic {} version {}, got magic {} version {}",
		    path.string(),
		    Formatting::FormatHexUInt32(kCookedShaderPackageMagic),
		    kCookedShaderPackageVersion,
		    Formatting::FormatHexUInt32(outPackage.m_header.Magic),
		    outPackage.m_header.Version);
		return false;
	}

	if (!reader.ReadArray(outPackage.m_header.BinaryRecordCount, outPackage.m_binaryRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.BindingRecordCount, outPackage.m_bindingRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.PipelineLayoutRecordCount, outPackage.m_pipelineLayoutRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.SpecializationInputCount, outPackage.m_specializationInputs, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ReflectionRecordCount, outPackage.m_reflectionRecords, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ResourceBindingRecordCount, outPackage.m_resourceBindings, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ConstantBufferRecordCount, outPackage.m_constantBuffers, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.ConstantBufferMemberRecordCount, outPackage.m_constantBufferMembers, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.InputElementRecordCount, outPackage.m_inputElements, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.PushConstantRangeRecordCount, outPackage.m_pushConstantRanges, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.SpecializationConstantRecordCount, outPackage.m_specializationConstants, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.RayTracingExportRecordCount, outPackage.m_rayTracingExports, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.RayTracingHitGroupRecordCount, outPackage.m_rayTracingHitGroups, outErrorMessage) ||
	    !reader
	         .ReadArray(outPackage.m_header.RayTracingLocalParameterRecordCount, outPackage.m_rayTracingLocalParameters, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.StringTableSizeInBytes, outPackage.m_stringTable, outErrorMessage) ||
	    !reader.ReadArray(outPackage.m_header.BinaryBlobSizeInBytes, outPackage.m_binaryBlob, outErrorMessage))
	{
		return false;
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		outErrorMessage = std::format("Cooked shader package '{}' contains unexpected trailing bytes", path.string());
		return false;
	}

	outPackage.m_isValid = true;
	outErrorMessage.clear();
	return true;
}

