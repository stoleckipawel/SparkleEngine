#include "PCH.h"

#include "Contracts/ShaderContractValidator.h"

#include <format>
#include <unordered_set>

namespace ShaderContractValidation
{
	ShaderContractVerificationFailure Failure(const ShaderContract& shader, std::string reason)
	{
		return ShaderContractVerificationFailure{
		    .shaderName = shader.shaderName,
		    .sourcePath = shader.sourcePath,
		    .entryPoint = shader.entryPoint,
		    .stage = shader.stage,
		    .reason = std::move(reason)};
	}
}

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
		if (!shader.hasParameterStruct && !IsRayTracingShaderStage(shader.stage))
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "missing-parameter-descriptor-builder"));
		}
		if (!shader.hasParameterStruct && shader.stage == ShaderStage::RayGeneration)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "ray-generation-missing-root-parameter-struct"));
		}
		if (shader.hasParameterStruct && IsRayTracingShaderStage(shader.stage) && shader.stage != ShaderStage::RayGeneration)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "non-ray-generation-root-parameter-struct"));
		}
		const bool hasSharedRayTracingContract = shader.rayTracing.PayloadSizeInBytes != 0
		    || shader.rayTracing.AttributeSizeInBytes != 0 || shader.rayTracing.MinimumRecursionDepth != 0;
		const bool hasLocalRecord = shader.rayTracing.LocalRecordSizeInBytes != 0 || shader.rayTracing.LocalRecordSignature != 0;
		if (shader.stage == ShaderStage::RayGeneration
		    && (shader.rayTracing.PayloadSizeInBytes == 0 || shader.rayTracing.AttributeSizeInBytes == 0
		        || shader.rayTracing.MinimumRecursionDepth == 0))
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "incomplete-ray-tracing-contract"));
		}
		if (IsRayTracingShaderStage(shader.stage) && shader.stage != ShaderStage::RayGeneration && hasSharedRayTracingContract)
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "shared-ray-tracing-contract-on-non-dispatch-stage"));
		}
		if (!IsRayTracingShaderStage(shader.stage) && (hasSharedRayTracingContract || hasLocalRecord))
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "ray-tracing-contract-on-non-ray-tracing-stage"));
		}
		if ((shader.rayTracing.LocalRecordSizeInBytes == 0) != (shader.rayTracing.LocalRecordSignature == 0))
		{
			failures.push_back(ShaderContractValidation::Failure(shader, "incomplete-local-record-contract"));
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
