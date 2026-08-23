#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoir.h"

#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Direct/DirectLightReservoirSpatialPass.h"
#include "Passes/Lighting/Direct/DirectLightReservoirTemporalPass.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    const RenderFrameGraphImportedSceneResources& externalResources)
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
		builder.AddParameterSetup<FrameUniformData>(
		    parameters,
		    [](auto& fields, const FrameUniformData& frame) { fields.Frame = frame; });
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
	};

	auto& temporalParameters = builder.AllocParameters<DirectLightReservoirTemporalPass::Parameters>();
	temporalParameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	temporalParameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	temporalParameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Previous);
	temporalParameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Previous);
	temporalParameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.ReservoirHistory.Surface.Previous);
	temporalParameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
	bindCommonParameters(temporalParameters);
	bindFrameParameters(temporalParameters);
	const auto invalidateTemporalHistory = [](auto& fields, bool hasBeenProduced)
	{
		if (!hasBeenProduced)
		{
			ViewTemporalUniformData temporal = *fields.ViewTemporal.GetValue();
			temporal.HistoryValid = 0u;
			fields.ViewTemporal = temporal;
		}
	};
	builder.AddResourceProductionSetup(
	    temporalParameters,
	    shadowSignals.ReservoirHistory.Sample.Previous,
	    invalidateTemporalHistory);
	builder.AddResourceProductionSetup(
	    temporalParameters,
	    shadowSignals.ReservoirHistory.Weight.Previous,
	    invalidateTemporalHistory);
	builder.AddResourceProductionSetup(
	    temporalParameters,
	    shadowSignals.ReservoirHistory.Surface.Previous,
	    invalidateTemporalHistory);
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
