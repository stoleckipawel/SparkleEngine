#include "PCH.h"

#include "Shaders/CookedShaderPackageCache.h"

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

LoadedShaderPackage CookedShaderPackageCache::LoadPackageFromFile(const std::filesystem::path& path)
{
	std::vector<std::uint8_t> fileBytes;
	std::string readError;
	if (!Files::TryReadAllBytes(path, fileBytes, readError))
	{
		throw Diagnostics::Error(std::move(readError));
	}

	LoadedShaderPackage package;
	Files::BinarySpanReader reader(fileBytes);
	if (!reader.ReadValue(package.m_header, readError))
	{
		throw Diagnostics::Error(std::move(readError));
	}

	if (!package.m_header.Matches(kCookedShaderPackageMagic, kCookedShaderPackageVersion))
	{
		if (package.m_header.Magic == kCookedShaderPackageMagic)
		{
			throw Diagnostics::Error(std::format(
			    "Cooked shader package '{}' is version {}. Recook to current version {}.",
			    path.string(),
			    package.m_header.Version,
			    kCookedShaderPackageVersion));
		}

		throw Diagnostics::Error(std::format(
		    "Invalid cooked shader package header in '{}': expected magic {} version {}, got magic {} version {}",
		    path.string(),
		    Formatting::FormatHexUInt32(kCookedShaderPackageMagic),
		    kCookedShaderPackageVersion,
		    Formatting::FormatHexUInt32(package.m_header.Magic),
		    package.m_header.Version));
	}

	if (!reader.ReadArray(package.m_header.BinaryRecordCount, package.m_binaryRecords, readError) ||
	    !reader.ReadArray(package.m_header.BindingRecordCount, package.m_bindingRecords, readError) ||
	    !reader.ReadArray(package.m_header.PipelineLayoutRecordCount, package.m_pipelineLayoutRecords, readError) ||
	    !reader.ReadArray(package.m_header.SpecializationInputCount, package.m_specializationInputs, readError) ||
	    !reader.ReadArray(package.m_header.ReflectionRecordCount, package.m_reflectionRecords, readError) ||
	    !reader.ReadArray(package.m_header.ResourceBindingRecordCount, package.m_resourceBindings, readError) ||
	    !reader.ReadArray(package.m_header.ConstantBufferRecordCount, package.m_constantBuffers, readError) ||
	    !reader.ReadArray(package.m_header.ConstantBufferMemberRecordCount, package.m_constantBufferMembers, readError) ||
	    !reader.ReadArray(package.m_header.InputElementRecordCount, package.m_inputElements, readError) ||
	    !reader.ReadArray(package.m_header.PushConstantRangeRecordCount, package.m_pushConstantRanges, readError) ||
	    !reader.ReadArray(package.m_header.SpecializationConstantRecordCount, package.m_specializationConstants, readError) ||
	    !reader.ReadArray(package.m_header.RayTracingExportRecordCount, package.m_rayTracingExports, readError) ||
	    !reader.ReadArray(package.m_header.RayTracingHitGroupRecordCount, package.m_rayTracingHitGroups, readError) ||
	    !reader
	         .ReadArray(package.m_header.RayTracingLocalParameterRecordCount, package.m_rayTracingLocalParameters, readError) ||
	    !reader.ReadArray(package.m_header.StringTableSizeInBytes, package.m_stringTable, readError) ||
	    !reader.ReadArray(package.m_header.BinaryBlobSizeInBytes, package.m_binaryBlob, readError))
	{
		throw Diagnostics::Error(std::move(readError));
	}

	if (reader.GetRemainingByteCount() != 0)
	{
		throw Diagnostics::Error(std::format("Cooked shader package '{}' contains unexpected trailing bytes", path.string()));
	}

	package.m_isValid = true;
	return package;
}
