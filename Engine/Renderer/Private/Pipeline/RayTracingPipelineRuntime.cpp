#include "PCH.h"

#include "Pipeline/RayTracingPipelineRuntime.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "PipelineRuntime/PipelineRuntimeLibrary.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Shaders/ShaderParameterLayoutBuilder.h"

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
		exports.push_back(
		    RhiRayTracingShaderExportDesc{
		        .Shader = &runtime->m_shaders.back(),
		        .ExportName = registration.EntryPoint});
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
		const ShaderRegistrationDesc* anyHit = group.AnyHit != 0
		    ? &RayTracingRuntimeMaterialization::GetRegistration(group.AnyHit)
		    : nullptr;
		const ShaderRegistrationDesc* intersection = group.Intersection != 0
		    ? &RayTracingRuntimeMaterialization::GetRegistration(group.Intersection)
		    : nullptr;
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

	std::vector<RhiRayTracingShaderRecord> rayGenerationRecords{
	    RhiRayTracingShaderRecord{.ExportName = rayGenerationRegistration.EntryPoint}};
	std::vector<RhiRayTracingShaderRecord> missRecords;
	std::vector<RhiRayTracingShaderRecord> hitGroupRecords;
	std::vector<RhiRayTracingShaderRecord> callableRecords;
	for (ShaderTypeId miss : composition.GetMissShaders())
	{
		missRecords.push_back(
		    RhiRayTracingShaderRecord{.ExportName = RayTracingRuntimeMaterialization::GetRegistration(miss).EntryPoint});
	}
	for (const RayTracingHitGroupComposition& group : composition.GetHitGroups())
	{
			hitGroupRecords.push_back(
		    RhiRayTracingShaderRecord{
		        .ExportName = group.ExportName,
		        .LocalData = group.LocalData,
		        .LocalRecordSignature =
		            RayTracingRuntimeMaterialization::GetRegistration(group.ClosestHit).RayTracing.LocalRecordSignature});
	}
	for (ShaderTypeId callable : composition.GetCallableShaders())
	{
		callableRecords.push_back(
		    RhiRayTracingShaderRecord{.ExportName = RayTracingRuntimeMaterialization::GetRegistration(callable).EntryPoint});
	}
	RayTracingShaderTableDesc tableDesc{
	    .Pipeline = runtime->m_pipeline.get(),
	    .RayGenerationRecords = rayGenerationRecords,
	    .MissRecords = missRecords,
	    .HitGroupRecords = hitGroupRecords,
	    .CallableRecords = callableRecords,
	    .Generation = generation,
	    .DebugName = debugName.c_str()};
	runtime->m_shaderTable = renderHardwareInterface.GetRayTracingService().CreateRayTracingShaderTable(tableDesc);
	if (runtime->m_shaderTable == nullptr)
	{
		throw Diagnostics::Error("Ray-tracing shader-table materialization failed.");
	}
	return runtime;
}
