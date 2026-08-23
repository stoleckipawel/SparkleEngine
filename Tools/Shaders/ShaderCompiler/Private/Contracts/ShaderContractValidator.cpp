#include "PCH.h"

#include "Contracts/ShaderContractValidator.h"

#include <format>
#include <unordered_map>
#include <unordered_set>

class ShaderContractValidatorImplementation final
{
  public:
	struct PackageValidationState final
	{
		std::string bindingLayoutId;
		CookedShaderPackageKind packageKind = CookedShaderPackageKind::Graphics;
		std::unordered_set<std::uint8_t> declaredStages;
	};

	static ShaderContractVerificationFailure BuildFailure(const ShaderContractStage& stage, std::string reason)
	{
		return ShaderContractVerificationFailure{
		    .shaderName = stage.shaderName,
		    .packageId = stage.packageId,
		    .bindingLayoutId = stage.bindingLayoutId,
		    .sourcePath = stage.sourcePath,
		    .entryPoint = stage.entryPoint,
		    .stage = stage.stage,
		    .reason = std::move(reason)};
	}
};

std::vector<ShaderContractVerificationFailure> ShaderContractValidator::Validate(const ShaderContractCatalog& catalog)
{
	std::vector<ShaderContractVerificationFailure> failures;
	std::unordered_set<std::string> shaderNames;
	std::unordered_map<std::string, ShaderContractValidatorImplementation::PackageValidationState> packages;

	for (const ShaderContractStage& stage : catalog.stages)
	{
		if (stage.shaderName.empty())
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "empty-shader-name"));
		}
		else if (!shaderNames.insert(stage.shaderName).second)
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "duplicate-shader-name"));
		}

		if (stage.packageId.empty())
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "empty-package-id"));
		}
		if (stage.bindingLayoutId.empty())
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "empty-binding-layout-id"));
		}
		if (stage.sourcePath.empty())
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "empty-source-path"));
		}
		if (stage.entryPoint.empty())
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "empty-entry-point"));
		}
		if (!stage.hasParameterStruct)
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "missing-parameter-descriptor-builder"));
		}

		const bool rayTracingLibrary = stage.packageKind == CookedShaderPackageKind::RayTracingLibrary;
		if (stage.stage == ShaderStage::Count && !rayTracingLibrary)
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "stage-count-is-only-valid-for-ray-tracing-library-packages"));
		}
		if (stage.stage != ShaderStage::Count && rayTracingLibrary)
		{
			failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "ray-tracing-library-package-must-use-library-stage"));
		}

		ShaderContractValidatorImplementation::PackageValidationState& packageState = packages[stage.packageId];
		if (packageState.bindingLayoutId.empty())
		{
			packageState.bindingLayoutId = stage.bindingLayoutId;
			packageState.packageKind = stage.packageKind;
		}
		else
		{
			if (packageState.bindingLayoutId != stage.bindingLayoutId)
			{
				failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "package-binding-layout-mismatch"));
			}
			if (packageState.packageKind != stage.packageKind)
			{
				failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "package-kind-mismatch"));
			}
		}

		if (!rayTracingLibrary)
		{
			const std::uint8_t stageKey = static_cast<std::uint8_t>(stage.stage);
			if (!packageState.declaredStages.insert(stageKey).second)
			{
				failures.push_back(ShaderContractValidatorImplementation::BuildFailure(stage, "duplicate-stage-in-package"));
			}
		}
	}

	return failures;
}

std::string ShaderContractValidator::FormatFailure(const ShaderContractVerificationFailure& failure)
{
	return std::format(
	    "shader={} package={} layout={} source={} entry={} stage={} reason={}",
	    failure.shaderName.empty() ? "<empty>" : failure.shaderName,
	    failure.packageId.empty() ? "<empty>" : failure.packageId,
	    failure.bindingLayoutId.empty() ? "<empty>" : failure.bindingLayoutId,
	    failure.sourcePath.empty() ? "<empty>" : failure.sourcePath,
	    failure.entryPoint.empty() ? "<empty>" : failure.entryPoint,
	    GetShaderStagePrefix(failure.stage),
	    failure.reason);
}
