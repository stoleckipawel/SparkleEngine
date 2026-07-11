#include "PCH.h"

#include "Shaders/ShaderRayTracingMetadataValidation.h"

#include "Shaders/CookedShaderPackageCache.h"

#include <algorithm>
#include <format>
#include <vector>

bool ShaderRayTracingMetadataValidation::ValidateInlineRayQueryMetadata(
    const LoadedShaderPackage& package,
    CookedShaderBinaryFormat requiredBinaryFormat,
    std::string& outErrorMessage)
{
	if (!package.IsValid())
	{
		outErrorMessage = "Cooked shader package payload is invalid.";
		return false;
	}

	const CookedShaderPackageHeader& header = package.GetHeader();
	const CookedShaderPackageFeatureFlags features = header.PackageFeatures;
	if (!HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesInlineRayQuery))
	{
		outErrorMessage = "Cooked shader package does not declare UsesInlineRayQuery.";
		return false;
	}
	if (!HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesAccelerationStructure))
	{
		outErrorMessage = "Cooked shader package declares inline ray query but not UsesAccelerationStructure.";
		return false;
	}
	if (header.PackageKind != CookedShaderPackageKind::Compute)
	{
		outErrorMessage = "Inline ray query validation currently expects a compute shader package.";
		return false;
	}
	const bool usesAccelerationStructureDeviceAddress =
	    HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesAccelerationStructureDeviceAddress);
	if (usesAccelerationStructureDeviceAddress && requiredBinaryFormat != CookedShaderBinaryFormat::SpirV)
	{
		outErrorMessage = "Acceleration-structure device-address inline ray query packages are currently supported only for SPIR-V.";
		return false;
	}

	bool hasRequiredBinary = false;
	for (const CookedShaderBinaryRecord& binary : package.GetBinaryRecords())
	{
		if (package.IsRuntimeBinary(binary, requiredBinaryFormat))
		{
			hasRequiredBinary = true;
			break;
		}
	}
	if (!hasRequiredBinary)
	{
		outErrorMessage = std::format(
		    "Inline ray query shader package has no binary for required target {}/{}.",
		    CookedShaderBinaryFormatToString(requiredBinaryFormat),
		    GetRuntimeShaderCodegenTarget(requiredBinaryFormat));
		return false;
	}

	bool hasAccelerationStructureLayoutBinding = false;
	for (const CookedShaderPipelineLayoutRecord& layout : package.GetPipelineLayoutRecords())
	{
		if (package.ResolveString(layout.CodegenTarget) == GetRuntimeShaderCodegenTarget(requiredBinaryFormat) &&
		    layout.AccelerationStructureCount > 0)
		{
			hasAccelerationStructureLayoutBinding = true;
			break;
		}
	}
	if (!hasAccelerationStructureLayoutBinding)
	{
		if (!usesAccelerationStructureDeviceAddress)
		{
			outErrorMessage = "Inline ray query shader package has no acceleration-structure binding in pipeline layout metadata.";
			return false;
		}
	}

	bool hasAccelerationStructureReflectionBinding = false;
	const std::vector<CookedShaderBinaryRecord>& binaries = package.GetBinaryRecords();
	const std::vector<CookedShaderReflectionRecord>& reflections = package.GetReflectionRecords();
	const std::vector<CookedShaderResourceBindingRecord>& bindings = package.GetResourceBindings();
	for (std::size_t reflectionIndex = 0;
	     reflectionIndex < reflections.size() && reflectionIndex < binaries.size() && !hasAccelerationStructureReflectionBinding;
	     ++reflectionIndex)
	{
		if (!package.IsRuntimeBinary(binaries[reflectionIndex], requiredBinaryFormat))
		{
			continue;
		}

		const CookedShaderReflectionRecord& reflection = reflections[reflectionIndex];
		const std::size_t bindingEnd = std::min<std::size_t>(
		    static_cast<std::size_t>(reflection.ResourceBindingOffset) + reflection.ResourceBindingCount,
		    bindings.size());
		for (std::size_t bindingIndex = reflection.ResourceBindingOffset; bindingIndex < bindingEnd; ++bindingIndex)
		{
			if (bindings[bindingIndex].Kind == CookedShaderResourceKind::AccelerationStructure)
			{
				hasAccelerationStructureReflectionBinding = true;
				break;
			}
		}
	}
	if (!hasAccelerationStructureReflectionBinding)
	{
		if (!usesAccelerationStructureDeviceAddress)
		{
			outErrorMessage = "Inline ray query shader package has no reflected acceleration-structure resource binding.";
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}
