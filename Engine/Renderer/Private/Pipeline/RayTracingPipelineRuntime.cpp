#include "PCH.h"

#include "Pipeline/RayTracingPipelineRuntime.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "PipelineRuntime/PipelineRuntimeLibrary.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Shaders/ShaderParameterLayoutBuilder.h"
#include "Scene/RayTracing/RayTracingShaderTablePlan.h"

#include <string_view>
#include <unordered_set>

namespace RayTracingRuntimeMaterialization
{
	const ShaderRegistrationDesc& GetRegistration(ShaderTypeId shaderType)
	{
		const ShaderRegistrationDesc* registration = GlobalShaderRegistry::FindById(shaderType);
		if (registration == nullptr)
		{
			throw Diagnostics::Error("Ray-tracing composition references an unregistered shader type.");
		}
		return *registration;
	}

}

std::unique_ptr<RayTracingPipelineRuntime> RayTracingPipelineRuntime::Create(
    RenderHardwareInterface& renderHardwareInterface,
    const GlobalShaderMap& map,
    const CookedShaderLibrary& library,
    ShaderTarget target,
    std::uint64_t generation,
    const RayTracingPipelineComposition& composition)
{
	if (!renderHardwareInterface.GetCapabilities().RayTracing.SupportsRayTracingPipeline || generation == 0)
	{
		throw Diagnostics::Error("Ray-tracing pipeline materialization requires complete backend readiness and a generation.");
	}
	auto runtime = std::make_unique<RayTracingPipelineRuntime>();
	runtime->m_generation = generation;

	std::vector<ShaderTypeId> selections;
	std::unordered_set<ShaderTypeId> selectedTypes;
	auto select = [&selections, &selectedTypes](ShaderTypeId shaderType)
	{
		if (selectedTypes.insert(shaderType).second)
		{
			selections.push_back(shaderType);
		}
	};
	select(composition.GetRayGeneration());
	for (ShaderTypeId miss : composition.GetMissShaders())
	{
		select(miss);
	}
	for (const RayTracingHitGroupComposition& group : composition.GetHitGroups())
	{
		select(group.ClosestHit);
		if (group.AnyHit != 0)
		{
			select(group.AnyHit);
		}
		if (group.Intersection != 0)
		{
			select(group.Intersection);
		}
	}
	for (ShaderTypeId callable : composition.GetCallableShaders())
	{
		select(callable);
	}
	runtime->m_shaders.reserve(selections.size());
	std::vector<RhiRayTracingShaderExportDesc> exports;
	exports.reserve(selections.size());
	for (ShaderTypeId selection : selections)
	{
		const ShaderRegistrationDesc& registration = RayTracingRuntimeMaterialization::GetRegistration(selection);
		const GlobalShaderMapEntry* entry = map.Find(selection, target);
		const CookedShaderCodeRecord* code = entry != nullptr ? library.Find(entry->CodeHash) : nullptr;
		runtime->m_shaders.push_back(ResolvedShader{.Map = &map, .Library = &library, .Entry = entry, .Code = code});
		exports.push_back(RhiRayTracingShaderExportDesc{.Shader = &runtime->m_shaders.back(), .ExportName = registration.EntryPoint});
		PipelineRuntimeLibrary::ValidateShaderCapabilities(renderHardwareInterface, registration.ShaderName, runtime->m_shaders.back());
	}

	const ShaderRegistrationDesc& rayGenerationRegistration =
	    RayTracingRuntimeMaterialization::GetRegistration(composition.GetRayGeneration());
	if (rayGenerationRegistration.BuildParameterStructDescriptor == nullptr)
	{
		throw Diagnostics::Error("Ray-generation shader has no registered global parameter contract.");
	}
	runtime->m_parameterLayout = BuildShaderParameterLayout(rayGenerationRegistration);
	std::wstring debugName = Strings::ToWide(rayGenerationRegistration.ShaderName);
	runtime->m_bindingLayout = PipelineRuntimeLibrary::CreateBindingLayout(
	    renderHardwareInterface,
	    runtime->m_parameterLayout,
	    std::span<const ResolvedShader>(runtime->m_shaders).first(1),
	    false,
	    debugName.c_str());

	std::vector<RhiRayTracingHitGroupDesc> hitGroups;
	hitGroups.reserve(composition.GetHitGroups().size());
	for (const RayTracingHitGroupComposition& group : composition.GetHitGroups())
	{
		const ShaderRegistrationDesc& closestHit = RayTracingRuntimeMaterialization::GetRegistration(group.ClosestHit);
		const ShaderRegistrationDesc* anyHit =
		    group.AnyHit != 0 ? &RayTracingRuntimeMaterialization::GetRegistration(group.AnyHit) : nullptr;
		const ShaderRegistrationDesc* intersection =
		    group.Intersection != 0 ? &RayTracingRuntimeMaterialization::GetRegistration(group.Intersection) : nullptr;
		hitGroups.push_back(
		    RhiRayTracingHitGroupDesc{
		        .ExportName = group.ExportName,
		        .Kind = group.Kind,
		        .ClosestHitExport = closestHit.EntryPoint,
		        .AnyHitExport = anyHit != nullptr ? anyHit->EntryPoint : std::string_view{},
		        .IntersectionExport = intersection != nullptr ? intersection->EntryPoint : std::string_view{}});
	}
	const RayTracingShaderMetadata& rayGenerationMetadata = rayGenerationRegistration.RayTracing;
	RayTracingPipelineDesc pipelineDesc{
	    .GlobalBindingLayout = runtime->m_bindingLayout.get(),
	    .ShaderExports = exports,
	    .HitGroups = hitGroups,
	    .MaxPayloadSizeInBytes = rayGenerationMetadata.PayloadSizeInBytes,
	    .MaxAttributeSizeInBytes = rayGenerationMetadata.AttributeSizeInBytes,
	    .MaxRecursionDepth = rayGenerationMetadata.MinimumRecursionDepth,
	    .Generation = generation,
	    .DebugName = debugName.c_str()};
	runtime->m_pipeline = renderHardwareInterface.GetPipelineService().CreateRayTracingPipeline(pipelineDesc);
	if (runtime->m_pipeline == nullptr)
	{
		throw Diagnostics::Error("Ray-tracing native pipeline materialization failed.");
	}

	return runtime;
}

std::unique_ptr<RayTracingShaderTable> RayTracingPipelineRuntime::CreateShaderTable(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingPipelineComposition& composition) const
{
	std::vector<const RayTracingHitGroupComposition*> recordGroups;
	recordGroups.reserve(composition.GetHitGroups().size());
	for (const RayTracingHitGroupComposition& group : composition.GetHitGroups())
	{
		recordGroups.push_back(&group);
	}
	return CreateShaderTable(renderHardwareInterface, composition, recordGroups);
}

std::unique_ptr<RayTracingShaderTable> RayTracingPipelineRuntime::CreateShaderTable(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingPipelineComposition& composition,
    const RayTracingShaderTablePlan& plan) const
{
	if (m_pipeline == nullptr || composition.GetRayGeneration() == 0u || !plan.Validate())
	{
		throw Diagnostics::Error("Ray-tracing shader-table materialization received an invalid pipeline or scene plan.");
	}

	const RayTracingHitGroupComposition* opaqueGroup = nullptr;
	const RayTracingHitGroupComposition* alphaTestedGroup = nullptr;
	for (const RayTracingHitGroupComposition& group : composition.GetHitGroups())
	{
		if (group.AnyHit == 0u)
		{
			if (opaqueGroup != nullptr)
			{
				throw Diagnostics::Error("Scene shader-table composition contains multiple opaque hit groups.");
			}
			opaqueGroup = &group;
		}
		else
		{
			if (alphaTestedGroup != nullptr)
			{
				throw Diagnostics::Error("Scene shader-table composition contains multiple alpha-tested hit groups.");
			}
			alphaTestedGroup = &group;
		}
	}
	if (opaqueGroup == nullptr || alphaTestedGroup == nullptr)
	{
		throw Diagnostics::Error("Scene shader-table materialization requires one opaque and one alpha-tested hit group.");
	}

	std::vector<const RayTracingHitGroupComposition*> recordGroups;
	recordGroups.reserve(plan.GetRecords().size());
	for (const RayTracingShaderTableRecordPlan& record : plan.GetRecords())
	{
		recordGroups.push_back(record.HitGroup == RayTracingShaderTableHitGroup::AlphaTested ? alphaTestedGroup : opaqueGroup);
	}
	return CreateShaderTable(renderHardwareInterface, composition, recordGroups);
}

std::unique_ptr<RayTracingShaderTable> RayTracingPipelineRuntime::CreateShaderTable(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingPipelineComposition& composition,
    std::span<const RayTracingHitGroupComposition* const> recordGroups) const
{
	if (m_pipeline == nullptr || composition.GetRayGeneration() == 0u)
	{
		throw Diagnostics::Error("Ray-tracing shader-table materialization received an invalid pipeline composition.");
	}
	const ShaderRegistrationDesc& rayGenerationRegistration =
	    RayTracingRuntimeMaterialization::GetRegistration(composition.GetRayGeneration());
	std::vector<RhiRayTracingShaderRecord> rayGenerationRecords{
	    RhiRayTracingShaderRecord{.ExportName = rayGenerationRegistration.EntryPoint}};
	std::vector<RhiRayTracingShaderRecord> missRecords;
	std::vector<RhiRayTracingShaderRecord> hitGroupRecords;
	std::vector<RhiRayTracingShaderRecord> callableRecords;
	missRecords.reserve(composition.GetMissShaders().size());
	hitGroupRecords.reserve(recordGroups.size());
	callableRecords.reserve(composition.GetCallableShaders().size());
	for (ShaderTypeId miss : composition.GetMissShaders())
	{
		missRecords.push_back(RhiRayTracingShaderRecord{.ExportName = RayTracingRuntimeMaterialization::GetRegistration(miss).EntryPoint});
	}
	for (const RayTracingHitGroupComposition* group : recordGroups)
	{
		if (group == nullptr)
		{
			throw Diagnostics::Error("Ray-tracing shader-table record has no hit-group composition.");
		}
		hitGroupRecords.push_back(
		    RhiRayTracingShaderRecord{
		        .ExportName = group->ExportName,
		        .LocalData = group->LocalData,
		        .LocalRecordSignature =
		            RayTracingRuntimeMaterialization::GetRegistration(group->ClosestHit).RayTracing.LocalRecordSignature});
	}
	for (ShaderTypeId callable : composition.GetCallableShaders())
	{
		callableRecords.push_back(
		    RhiRayTracingShaderRecord{.ExportName = RayTracingRuntimeMaterialization::GetRegistration(callable).EntryPoint});
	}
	std::wstring debugName = Strings::ToWide(rayGenerationRegistration.ShaderName);
	RayTracingShaderTableDesc tableDesc{
	    .Pipeline = m_pipeline.get(),
	    .RayGenerationRecords = rayGenerationRecords,
	    .MissRecords = missRecords,
	    .HitGroupRecords = hitGroupRecords,
	    .CallableRecords = callableRecords,
	    .Generation = m_generation,
	    .DebugName = debugName.c_str()};
	std::unique_ptr<RayTracingShaderTable> shaderTable =
	    renderHardwareInterface.GetRayTracingService().CreateRayTracingShaderTable(tableDesc);
	if (shaderTable == nullptr)
	{
		throw Diagnostics::Error("Ray-tracing shader-table materialization failed.");
	}
	return shaderTable;
}
