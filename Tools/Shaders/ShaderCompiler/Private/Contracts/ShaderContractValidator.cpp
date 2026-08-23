#include "PCH.h"

#include "Contracts/ShaderContractValidator.h"

#include <format>
#include <unordered_set>

class ShaderContractValidation final
{
public:
	static ShaderContractVerificationFailure Failure(const ShaderContract& shader, std::string reason)
	{
		return ShaderContractVerificationFailure{
		    .shaderName = shader.shaderName,
		    .sourcePath = shader.sourcePath,
		    .entryPoint = shader.entryPoint,
		    .stage = shader.stage,
		    .reason = std::move(reason)};
	}
};

std::vector<ShaderContractVerificationFailure> ShaderContractValidator::Validate(const ShaderContractCatalog& catalog)
{
	std::vector<ShaderContractVerificationFailure> failures;
	std::unordered_set<std::string> names;
	std::unordered_set<ShaderTypeId> typeIds;
	for (const ShaderContract& shader : catalog)
	{
		if (shader.shaderName.empty() || !names.insert(shader.shaderName).second)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "missing-or-duplicate-shader-name"));
		}
		if (shader.shaderTypeId == 0 || !typeIds.insert(shader.shaderTypeId).second)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "missing-or-duplicate-shader-type-id"));
		}
		if (shader.sourcePath.empty())
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "empty-source-path"));
		}
		if (shader.entryPoint.empty())
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "empty-entry-point"));
		}
		if (shader.stage == ShaderStage::Count)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "unsupported-stage"));
		}
		if (!shader.hasParameterStruct)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "missing-parameter-descriptor-builder"));
		}
	}
	return failures;
}

std::string ShaderContractValidator::FormatFailure(const ShaderContractVerificationFailure& failure)
{
	return std::format(
	    "shader={} source={} entry={} stage={} reason={}",
	    failure.shaderName.empty() ? "<empty>" : failure.shaderName,
	    failure.sourcePath.empty() ? "<empty>" : failure.sourcePath,
	    failure.entryPoint.empty() ? "<empty>" : failure.entryPoint,
	    GetShaderStagePrefix(failure.stage),
	    failure.reason);
}
