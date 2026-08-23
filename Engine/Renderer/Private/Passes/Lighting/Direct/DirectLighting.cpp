#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLighting.h"

#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Direct/DirectLightingPass.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddDirectLightingPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const LightingRenderTargets& lighting,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const RenderFrameGraphImportedSceneResources& externalResources)
{
	auto& parameters = builder.AllocParameters<DirectLightingPass::Parameters>();
	parameters->DirectDiffuse = builder.CreateUAV(lighting.DirectDiffuse);
	parameters->DirectSpecular = builder.CreateUAV(lighting.DirectSpecular);
	parameters->DirectSubsurface = builder.CreateUAV(lighting.DirectSubsurface);
	parameters->ShadowVisibilitySignal = builder.CreateSRV(shadowSignals.Visibility);
	parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
	parameters->DirectionalLights = builder.CreateSRV(externalResources.Scene.Lighting.DirectionalLights);
	parameters->PointLights = builder.CreateSRV(externalResources.Scene.Lighting.PointLights);
	parameters->SpotLights = builder.CreateSRV(externalResources.Scene.Lighting.SpotLights);
	parameters->RectLights = builder.CreateSRV(externalResources.Scene.Lighting.RectLights);
	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [](auto& fields, const RenderView& view)
	    {
		    fields.View = view.uniform;
		    fields.ViewCamera = view.cameraUniform;
		    fields.ViewTemporal = view.temporalUniform;
	    });
	builder.AddParameterSetup<PreparedRenderScene>(
	    parameters,
	    [](auto& fields, const PreparedRenderScene& scene) { fields.SceneLighting = scene.gpuBindings->Lighting.Uniform; });
	builder.Dispatch<DirectLightingPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
