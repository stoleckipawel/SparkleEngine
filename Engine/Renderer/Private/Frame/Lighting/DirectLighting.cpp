#include "../../PCH.h"
#include "Frame/Lighting/DirectLighting.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightingPass.h"
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
    const FrameAssemblyExternalResources& externalResources)
{
	auto& parameters = builder.AllocParameters<DirectLightingPass::Parameters>();
	auto* parameterFields = parameters.operator->();
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
	builder.AddFrameUniformSetup([parameterFields](const FrameUniformData& frame) { parameterFields->Frame = frame; });
	builder.AddRenderViewSetup(
	    [parameterFields](const RenderView& view)
	    {
		    parameterFields->View = view.uniform;
		    parameterFields->ViewCamera = view.cameraUniform;
		    parameterFields->ViewTemporal = view.temporalUniform;
	    });
	builder.AddPreparedSceneSetup(
	    [parameterFields](const PreparedRenderScene& scene) { parameterFields->SceneLighting = scene.gpuBindings->Lighting.Uniform; });
	builder.Dispatch<DirectLightingPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
