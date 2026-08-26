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
#include "RayTracing/Effects/Shadows/RayTracingShadowExecutionPlan.h"
#include "RayTracing/RayTracingPipelineComposition.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "View/RenderView.h"

#include <string>
#include <vector>

namespace DirectShadowSignalPasses
{
	template <typename TShader>
	auto& BuildParameters(
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
		descriptorParameters->MaterialTextureSampler = RhiSamplerDesc{
		    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
		    .MipFilter = RhiSamplerMipFilter::Linear,
		    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
		    .MaxAnisotropy = RhiSamplerAnisotropy::X1};
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
	    const RenderFrameGraphImportedSceneResources& externalResources,
	    const char* reason)
	{
		auto& parameters = BuildParameters<DirectShadowSignalCS>(
		    builder,
		    sceneTargets,
		    gbuffer,
		    sceneTlas,
		    shadowSignals,
		    externalResources);
		builder.Dispatch<DirectShadowSignalCS>(
		    std::string("DirectShadowSignal.Inline.") + reason,
		    parameters,
		    ComputeDispatchDesc{
		        MathUtils::DivideRoundUp(sceneExtent.Width, 8u),
		        MathUtils::DivideRoundUp(sceneExtent.Height, 8u),
		        1u});
	}

	void AddPipeline(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent sceneExtent,
	    const SceneRenderTargets& sceneTargets,
	    const GBufferRenderTargets& gbuffer,
	    FrameGraphAccelerationStructureHandle sceneTlas,
	    const DirectShadowSignalResources& shadowSignals,
	    const RenderFrameGraphImportedSceneResources& externalResources,
	    RayTracingShaderTablePlan& shaderTablePlan,
	    const char* reason)
	{
		auto& parameters = BuildParameters<DirectShadowSignalRGS>(
		    builder,
		    sceneTargets,
		    gbuffer,
		    sceneTlas,
		    shadowSignals,
		    externalResources);
		const RayTracingPipelineComposition composition = RayTracingPipelineComposition::Create<DirectShadowSignalRGS>(
		    std::vector{RayTracingPipelineComposition::Shader<DirectShadowSignalMiss>()},
		    std::vector{
		        RayTracingHitGroupComposition::Triangles<DirectShadowSignalClosestHit>("DirectShadowSignalOpaqueHitGroup"),
		        RayTracingHitGroupComposition::Triangles<DirectShadowSignalClosestHit, DirectShadowSignalAnyHit>(
		            "DirectShadowSignalAlphaTestedHitGroup")});
		builder.TraceRays<DirectShadowSignalRGS>(
		    std::string("DirectShadowSignal.Pipeline.") + reason,
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
	const RayTracingShadowExecutionPlan executionPlan = ResolveRayTracingShadowExecutionPlan(rayTracingScene.GetCapabilityReport());
	const char* reason = GetRayTracingShadowExecutionReasonLabel(executionPlan.Reason);
	switch (executionPlan.Active)
	{
		case RayTracingExecutionFrontend::Inline:
			DirectShadowSignalPasses::AddInline(
			    builder,
			    sceneExtent,
			    sceneTargets,
			    gbuffer,
			    sceneTlas,
			    shadowSignals,
			    externalResources,
			    reason);
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
			    rayTracingScene.GetShaderTablePlan(),
			    reason);
			return;
		case RayTracingExecutionFrontend::None:
		default:
			throw Diagnostics::Error("Direct-shadow graph construction has no real ray-tracing producer.");
	}
}
