#pragma once

#include "Core/Public/Diagnostics/Error.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/RayTracingExecutionFrontend.h"
#include "RayTracing/RayTracingMaterialPipelineShaders.h"
#include "RayTracing/RayTracingPipelineComposition.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"

#include <string_view>
#include <vector>

template <typename TInlineShader, typename TRayGenerationShader, typename TBuildParameters> void AddRayTracingMaterialPass(
    FrameGraphBuilder& builder,
    std::string_view label,
    RenderRayTracingScene& scene,
    ComputeDispatchDesc inlineDimensions,
    RayTracingDispatchDimensions pipelineDimensions,
    TBuildParameters&& buildParameters)
{
	switch (scene.GetExecutionFrontend())
	{
		case RayTracingExecutionFrontend::Inline:
		{
			auto& parameters = buildParameters.template operator()<TInlineShader>();
			builder.Dispatch<TInlineShader>(label, parameters, inlineDimensions);
			return;
		}
		case RayTracingExecutionFrontend::Pipeline:
		{
			static const RayTracingPipelineComposition composition = RayTracingPipelineComposition::Create<TRayGenerationShader>(
			    std::vector{RayTracingPipelineComposition::Shader<RayTracingMaterialMiss>()},
			    std::vector{
			        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit>("RayTracingMaterialOpaqueHitGroup"),
			        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit, RayTracingMaterialAnyHit>(
			            "RayTracingMaterialAlphaTestedHitGroup")});
			auto& parameters = buildParameters.template operator()<TRayGenerationShader>();
			builder.TraceRays<TRayGenerationShader>(label, composition, scene.GetShaderTablePlan(), parameters, pipelineDimensions);
			return;
		}
		case RayTracingExecutionFrontend::None:
		default:
			throw Diagnostics::Error("Ray-tracing material pass has no supported execution frontend.");
	}
}
