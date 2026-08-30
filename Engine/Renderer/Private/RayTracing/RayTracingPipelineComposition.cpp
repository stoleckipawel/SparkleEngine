#include "PCH.h"

#include "RayTracing/RayTracingPipelineComposition.h"

#include "Core/Public/Diagnostics/Error.h"

#include <unordered_map>
#include <unordered_set>

namespace RayTracingCompositionValidation
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw Diagnostics::Error(std::string(message));
		}
	}

	const RayTracingShaderMetadata& GetRayGenerationMetadata(ShaderTypeId shaderType)
	{
		const ShaderRegistrationDesc* registration = GlobalShaderRegistry::FindById(shaderType);
		Require(
		    registration != nullptr && registration->Stage == ShaderStage::RayGeneration,
		    "Ray-tracing composition requires a registered ray-generation shader.");
		return registration->RayTracing;
	}

	void ValidateShader(
	    ShaderTypeId shader,
	    ShaderStage expectedStage,
	    std::unordered_map<ShaderTypeId, ShaderStage>& shadersByType,
	    std::unordered_map<std::string, ShaderTypeId>& shaderTypesByExport)
	{
		const ShaderRegistrationDesc* registration = GlobalShaderRegistry::FindById(shader);
		Require(
		    registration != nullptr && registration->Stage == expectedStage && !registration->ShaderName.empty()
		        && !registration->EntryPoint.empty(),
		    "Ray-tracing composition contains an invalid typed shader selection.");
		const auto [shaderByType, insertedType] = shadersByType.emplace(shader, expectedStage);
		Require(
		    insertedType || shaderByType->second == expectedStage,
		    "One ray-tracing shader type resolves to conflicting composition contracts.");
		const auto [typeByExport, insertedExport] = shaderTypesByExport.emplace(std::string(registration->EntryPoint), shader);
		Require(insertedExport || typeByExport->second == shader, "One ray-tracing export name resolves to multiple shader types.");
	}
}

RayTracingPipelineComposition::RayTracingPipelineComposition(
    ShaderTypeId rayGeneration,
    std::vector<ShaderTypeId> missShaders,
    std::vector<RayTracingHitGroupComposition> hitGroups,
    std::vector<ShaderTypeId> callableShaders) :
    m_rayGeneration(rayGeneration),
    m_missShaders(std::move(missShaders)),
    m_hitGroups(std::move(hitGroups)),
    m_callableShaders(std::move(callableShaders))
{
	const RayTracingShaderMetadata& rayGenerationMetadata = RayTracingCompositionValidation::GetRayGenerationMetadata(m_rayGeneration);
	std::unordered_map<ShaderTypeId, ShaderStage> shadersByType;
	std::unordered_map<std::string, ShaderTypeId> shaderTypesByExport;
	RayTracingCompositionValidation::ValidateShader(m_rayGeneration, ShaderStage::RayGeneration, shadersByType, shaderTypesByExport);
	for (ShaderTypeId miss : m_missShaders)
	{
		RayTracingCompositionValidation::ValidateShader(miss, ShaderStage::Miss, shadersByType, shaderTypesByExport);
	}
	for (ShaderTypeId callable : m_callableShaders)
	{
		RayTracingCompositionValidation::ValidateShader(callable, ShaderStage::Callable, shadersByType, shaderTypesByExport);
	}
	std::unordered_set<std::string> hitGroupNames;
	for (const RayTracingHitGroupComposition& group : m_hitGroups)
	{
		RayTracingCompositionValidation::Require(
		    !group.ExportName.empty() && shaderTypesByExport.find(group.ExportName) == shaderTypesByExport.end()
		        && hitGroupNames.insert(group.ExportName).second,
		    "Ray-tracing composition contains an invalid or duplicate hit-group export.");
		RayTracingCompositionValidation::ValidateShader(group.ClosestHit, ShaderStage::ClosestHit, shadersByType, shaderTypesByExport);
		if (group.AnyHit != 0)
		{
			RayTracingCompositionValidation::ValidateShader(group.AnyHit, ShaderStage::AnyHit, shadersByType, shaderTypesByExport);
		}
		if (group.Kind == ERhiRayTracingHitGroupKind::Procedural)
		{
			RayTracingCompositionValidation::ValidateShader(
			    group.Intersection,
			    ShaderStage::Intersection,
			    shadersByType,
			    shaderTypesByExport);
		}
		else
		{
			RayTracingCompositionValidation::Require(group.Intersection == 0, "Triangle hit group contains an intersection shader.");
		}
		const RayTracingShaderMetadata& localOwner = GlobalShaderRegistry::FindById(group.ClosestHit)->RayTracing;
		RayTracingCompositionValidation::Require(
		    group.LocalData.size() == localOwner.LocalRecordSizeInBytes,
		    "Ray-tracing hit-group local data does not match its declared record schema.");
	}
	RayTracingCompositionValidation::Require(
	    rayGenerationMetadata.PayloadSizeInBytes != 0 && rayGenerationMetadata.AttributeSizeInBytes != 0
	        && rayGenerationMetadata.MinimumRecursionDepth != 0,
	    "Ray-tracing composition has an incomplete payload, attribute, or recursion contract.");
}
