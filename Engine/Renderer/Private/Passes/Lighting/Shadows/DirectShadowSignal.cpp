#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalShader.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"
#include "RayTracing/Effects/RayTracingExecutionFrontend.h"
#include "RayTracing/RayTracingMaterialPipelineShaders.h"
#include "RayTracing/RayTracingPipelineComposition.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "View/RenderView.h"

#include <vector>

namespace DirectShadowSignalPasses
{
	template <typename TShader> auto& BuildParameters(
	    FrameGraphBuilder& builder,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const DirectShadowSignalResources& shadowSignals,
	    const RenderFrameGraphImportedSceneResources& externalResources)
	{
		auto& descriptorParameters = builder.AllocParameters<TShader>();
		descriptorParameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
		descriptorParameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
		descriptorParameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
		descriptorParameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
		descriptorParameters->DirectionalLights = builder.CreateSRV(externalResources.Scene.Lighting.DirectionalLights);
		descriptorParameters->PointLights = builder.CreateSRV(externalResources.Scene.Lighting.PointLights);
		descriptorParameters->SpotLights = builder.CreateSRV(externalResources.Scene.Lighting.SpotLights);
		descriptorParameters->RectLights = builder.CreateSRV(externalResources.Scene.Lighting.RectLights);
		descriptorParameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		descriptorParameters->RayTracingHitVertices = builder.CreateSRV(externalResources.Scene.RayTracing.Vertices);
		descriptorParameters->RayTracingHitIndices = builder.CreateSRV(externalResources.Scene.RayTracing.Indices);
		descriptorParameters->RayTracingHitInstances = builder.CreateSRV(externalResources.Scene.RayTracing.Instances);
		descriptorParameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.Scene.RayTracing.Materials);
		descriptorParameters->SceneTlas = builder.CreateAccelerationStructureBinding(sceneTlas);
		builder.AddParameterSetup<RenderView>(
		    descriptorParameters,
		    [](auto& fields, const RenderView& view)
		    {
			    fields.View = view.uniform;
			    fields.ViewCamera = view.cameraUniform;
			    fields.ViewTemporal = view.temporalUniform;
		    });
		builder.AddParameterSetup<PreparedRenderScene>(
		    descriptorParameters,
		    [](auto& fields, const PreparedRenderScene& scene)
		    {
			    fields.SceneLighting = scene.gpuBindings->Lighting.Uniform;
			    fields.MaterialTextureTable = scene.materialTextureTable.Binding;
		    });
		builder.AddParameterSetup<RayTracedShadowPassInput>(
		    descriptorParameters,
		    [](auto& fields, const RayTracedShadowPassInput& input)
		    { fields.RayTracedShadowConstants = RayTracedShadowPassData::Build(input); });
		return descriptorParameters;
	}

	void AddInline(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const DirectShadowSignalResources& shadowSignals,
	    const RenderFrameGraphImportedSceneResources& externalResources)
	{
		auto& parameters =
		    BuildParameters<DirectShadowSignalCS>(builder, sceneTargets, gbuffer, sceneTlas, shadowSignals, externalResources);
		builder.Dispatch<DirectShadowSignalCS>(
		    "DirectShadowSignal.Inline",
		    parameters,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
	}

	void AddPipeline(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const DirectShadowSignalResources& shadowSignals,
	    const RenderFrameGraphImportedSceneResources& externalResources,
	    RayTracingShaderTablePlan& shaderTablePlan)
	{
		auto& parameters =
		    BuildParameters<DirectShadowSignalRGS>(builder, sceneTargets, gbuffer, sceneTlas, shadowSignals, externalResources);
		const RayTracingPipelineComposition composition = RayTracingPipelineComposition::Create<DirectShadowSignalRGS>(
		    std::vector{RayTracingPipelineComposition::Shader<RayTracingMaterialMiss>()},
		    std::vector{
		        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit>("RayTracingMaterialOpaqueHitGroup"),
		        RayTracingHitGroupComposition::Triangles<RayTracingMaterialClosestHit, RayTracingMaterialAnyHit>(
		            "RayTracingMaterialAlphaTestedHitGroup")});
		builder.TraceRays<DirectShadowSignalRGS>(
		    "DirectShadowSignal.Pipeline",
		    composition,
		    shaderTablePlan,
		    parameters,
		    RayTracingDispatchDimensions{.Width = sceneExtent.Width, .Height = sceneExtent.Height, .Depth = 1u});
	}
}

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const DirectShadowSignalResources& shadowSignals,
    const RenderFrameGraphImportedSceneResources& externalResources,
    RenderRayTracingScene& rayTracingScene)
{
	switch (ResolveRayTracingExecutionFrontend(rayTracingScene.GetCapabilityReport()))
	{
		case RayTracingExecutionFrontend::Inline:
			DirectShadowSignalPasses::AddInline(builder, sceneExtent, sceneTargets, gbuffer, sceneTlas, shadowSignals, externalResources);
			return;
		case RayTracingExecutionFrontend::Pipeline:
			DirectShadowSignalPasses::AddPipeline(
			    builder,
			    sceneExtent,
			    sceneTargets,
			    gbuffer,
			    sceneTlas,
			    shadowSignals,
			    externalResources,
			    rayTracingScene.GetShaderTablePlan());
			return;
		case RayTracingExecutionFrontend::None:
		default:
			throw Diagnostics::Error("Direct-shadow graph construction has no real ray-tracing producer.");
	}
}
