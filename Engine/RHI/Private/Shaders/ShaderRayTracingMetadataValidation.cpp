#include "PCH.h"

#include "Shaders/ShaderRayTracingMetadataValidation.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Shaders/LoadedShaderPackage.h"

#include <algorithm>
#include <format>
#include <vector>

void ShaderRayTracingMetadataValidation::ValidateInlineRayQueryMetadata(
    const LoadedShaderPackage& package,
    CookedShaderBinaryFormat runtimeBinaryFormat)
{
	if (!package.IsValid())
	{
		throw Diagnostics::Error("Cooked shader package payload is invalid.");
	}

	const CookedShaderPackageHeader& header = package.GetHeader();
	const CookedShaderPackageFeatureFlags features = header.PackageFeatures;
	if (!HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesInlineRayQuery))
	{
		throw Diagnostics::Error("Cooked shader package does not declare UsesInlineRayQuery.");
	}
	if (!HasCookedShaderPackageFeature(features, CookedShaderPackageFeatureFlags::UsesAccelerationStructure))
	{
		throw Diagnostics::Error("Cooked shader package declares inline ray query but not UsesAccelerationStructure.");
	}
	if (header.PackageKind != CookedShaderPackageKind::Compute)
	{
		throw Diagnostics::Error("Inline ray query validation currently expects a compute shader package.");
	}
	bool hasRuntimeBinary = false;
	for (const CookedShaderBinaryRecord& binary : package.GetBinaryRecords())
	{
		if (package.IsRuntimeBinary(binary, runtimeBinaryFormat))
		{
			hasRuntimeBinary = true;
			break;
		}
	}
	if (!hasRuntimeBinary)
	{
		throw Diagnostics::Error(std::format(
		    "Inline ray query shader package has no binary for runtime target {}/{}.",
		    CookedShaderBinaryFormatToString(runtimeBinaryFormat),
		    GetRuntimeShaderCodegenTarget(runtimeBinaryFormat)));
	}

	bool hasAccelerationStructureLayoutBinding = false;
	for (const CookedShaderPipelineLayoutRecord& layout : package.GetPipelineLayoutRecords())
	{
		if (package.ResolveString(layout.CodegenTarget) == GetRuntimeShaderCodegenTarget(runtimeBinaryFormat) &&
		    layout.AccelerationStructureCount > 0)
		{
			hasAccelerationStructureLayoutBinding = true;
			break;
		}
	}
	if (!hasAccelerationStructureLayoutBinding)
	{
		throw Diagnostics::Error(
		    "Inline ray query shader package has no acceleration-structure binding in pipeline layout metadata.");
	}

	bool hasAccelerationStructureReflectionBinding = false;
	const std::vector<CookedShaderBinaryRecord>& binaries = package.GetBinaryRecords();
	const std::vector<CookedShaderReflectionRecord>& reflections = package.GetReflectionRecords();
	const std::vector<CookedShaderResourceBindingRecord>& bindings = package.GetResourceBindings();
	for (std::size_t reflectionIndex = 0;
	     reflectionIndex < reflections.size() && reflectionIndex < binaries.size() && !hasAccelerationStructureReflectionBinding;
	     ++reflectionIndex)
	{
		if (!package.IsRuntimeBinary(binaries[reflectionIndex], runtimeBinaryFormat))
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
		throw Diagnostics::Error(
		    "Inline ray query shader package has no reflected acceleration-structure resource binding.");
	}
}
