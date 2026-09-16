#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoir.h"

#include "Core/Public/Math/MathUtils.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Direct/DirectLightReservoirSpatialShader.h"
#include "Passes/Lighting/Direct/DirectLightReservoirTemporalShader.h"
#include "ShaderData/SceneShaderParameters.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals)
{
	const GBufferRenderTargets& gbuffer = resources.Transient.GBuffer;
	const auto bindCommonParameters = [&](auto& parameters)
	{
		parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
		parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
		BindSceneShaderParameters(builder, parameters, resources);
	};

	auto& temporalParameters = builder.AllocParameters<DirectLightReservoirTemporalCS>();
	temporalParameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	temporalParameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	temporalParameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Previous);
	temporalParameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Previous);
	temporalParameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.ReservoirHistory.Surface.Previous);
	temporalParameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
	bindCommonParameters(temporalParameters);
	const auto invalidateTemporalHistory = [](auto& fields, bool hasBeenProduced)
	{
		if (!hasBeenProduced)
		{
			ViewTemporalUniformData temporal = *fields.ViewTemporal.GetValue();
			temporal.HistoryValid = 0u;
			fields.ViewTemporal = temporal;
		}
	};
	builder.AddResourceProductionSetup(temporalParameters, shadowSignals.ReservoirHistory.Sample.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(temporalParameters, shadowSignals.ReservoirHistory.Weight.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(temporalParameters, shadowSignals.ReservoirHistory.Surface.Previous, invalidateTemporalHistory);
	builder.Dispatch<DirectLightReservoirTemporalCS>(
	    temporalParameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});

	auto& spatialParameters = builder.AllocParameters<DirectLightReservoirSpatialCS>();
	spatialParameters->TemporalReservoirSample = builder.CreateSRV(shadowSignals.TemporalReservoirSample);
	spatialParameters->TemporalReservoirWeight = builder.CreateSRV(shadowSignals.TemporalReservoirWeight);
	spatialParameters->CurrentReservoirSample = builder.CreateUAV(shadowSignals.ReservoirHistory.Sample.Current);
	spatialParameters->CurrentReservoirWeight = builder.CreateUAV(shadowSignals.ReservoirHistory.Weight.Current);
	spatialParameters->CurrentReservoirSurface = builder.CreateUAV(shadowSignals.ReservoirHistory.Surface.Current);
	bindCommonParameters(spatialParameters);
	builder.Dispatch<DirectLightReservoirSpatialCS>(
	    spatialParameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
