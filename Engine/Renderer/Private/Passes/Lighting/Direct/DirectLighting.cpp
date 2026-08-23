#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLighting.h"

#include "Core/Public/Math/MathUtils.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
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
	auto& parameters = builder.AllocParameters<DirectLightingCS>();
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
	builder.Dispatch<DirectLightingCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
