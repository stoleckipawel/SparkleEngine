#include "PCH.h"

#include "Shaders/ShaderRayTracingMetadataValidation.h"

#include "Shaders/CookedShaderPackageCache.h"

#include <format>

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

	bool hasRequiredBinary = false;
	for (const CookedShaderBinaryRecord& binary : package.GetBinaryRecords())
	{
		if (binary.Format == requiredBinaryFormat)
		{
			hasRequiredBinary = true;
			break;
		}
	}
	if (!hasRequiredBinary)
	{
		outErrorMessage = std::format(
		    "Inline ray query shader package has no binary for required format {}.",
		    CookedShaderBinaryFormatToString(requiredBinaryFormat));
		return false;
	}

	bool hasAccelerationStructureLayoutBinding = false;
	for (const CookedShaderPipelineLayoutRecord& layout : package.GetPipelineLayoutRecords())
	{
		if (layout.AccelerationStructureCount > 0)
		{
			hasAccelerationStructureLayoutBinding = true;
			break;
		}
	}
	if (!hasAccelerationStructureLayoutBinding)
	{
		outErrorMessage = "Inline ray query shader package has no acceleration-structure binding in pipeline layout metadata.";
		return false;
	}

	bool hasAccelerationStructureReflectionBinding = false;
	for (const CookedShaderResourceBindingRecord& binding : package.GetResourceBindings())
	{
		if (binding.Kind == CookedShaderResourceKind::AccelerationStructure)
		{
			hasAccelerationStructureReflectionBinding = true;
			break;
		}
	}
	if (!hasAccelerationStructureReflectionBinding)
	{
		outErrorMessage = "Inline ray query shader package has no reflected acceleration-structure resource binding.";
		return false;
	}

	outErrorMessage.clear();
	return true;
}
