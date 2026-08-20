#include "../../PCH.h"
#include "Frame/Lighting/DirectLightReservoir.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightReservoirSpatialPass.h"
#include "Passes/Deferred/DirectLightReservoirTemporalPass.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const FrameAssemblyExternalResources& externalResources)
{
	const auto bindCommonParameters = [&](auto& parameters)
	{
		parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
		parameters->SceneDepth = builder.CreateSRV(sceneTargets.SceneDepth);
		parameters->DirectionalLights = builder.CreateSRV(externalResources.Scene.Lighting.DirectionalLights);
		parameters->PointLights = builder.CreateSRV(externalResources.Scene.Lighting.PointLights);
		parameters->SpotLights = builder.CreateSRV(externalResources.Scene.Lighting.SpotLights);
		parameters->RectLights = builder.CreateSRV(externalResources.Scene.Lighting.RectLights);
	};
	const auto bindFrameParameters = [&](auto& parameters)
	{
		auto* parameterFields = parameters.operator->();
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
	};

	auto& temporalParameters = builder.AllocParameters<DirectLightReservoirTemporalPass::Parameters>();
	auto* temporalFields = temporalParameters.operator->();
	temporalParameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	temporalParameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	temporalParameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Previous);
	temporalParameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Previous);
	temporalParameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.ReservoirHistory.Surface.Previous);
	temporalParameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
	bindCommonParameters(temporalParameters);
	bindFrameParameters(temporalParameters);
	builder.AddDirectLightReservoirHistorySetup(
	    [temporalFields](bool historyValid)
	    {
		    if (!historyValid)
		    {
			    ViewTemporalUniformData temporal = *temporalFields->ViewTemporal.GetValue();
			    temporal.HistoryValid = 0u;
			    temporalFields->ViewTemporal = temporal;
		    }
	    });
	builder.Dispatch<DirectLightReservoirTemporalPass>(temporalParameters, sceneExtent.Width, sceneExtent.Height);

	auto& spatialParameters = builder.AllocParameters<DirectLightReservoirSpatialPass::Parameters>();
	spatialParameters->TemporalReservoirSample = builder.CreateSRV(shadowSignals.TemporalReservoirSample);
	spatialParameters->TemporalReservoirWeight = builder.CreateSRV(shadowSignals.TemporalReservoirWeight);
	spatialParameters->CurrentReservoirSample = builder.CreateUAV(shadowSignals.ReservoirHistory.Sample.Current);
	spatialParameters->CurrentReservoirWeight = builder.CreateUAV(shadowSignals.ReservoirHistory.Weight.Current);
	spatialParameters->CurrentReservoirSurface = builder.CreateUAV(shadowSignals.ReservoirHistory.Surface.Current);
	bindCommonParameters(spatialParameters);
	bindFrameParameters(spatialParameters);
	builder.Dispatch<DirectLightReservoirSpatialPass>(spatialParameters, sceneExtent.Width, sceneExtent.Height);
}
