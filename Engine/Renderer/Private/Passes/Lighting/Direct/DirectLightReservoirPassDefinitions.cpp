#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPassDefinitions.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Direct/DirectLightReservoirSpatialShader.h"
#include "Passes/Lighting/Direct/DirectLightReservoirTemporalShader.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalResources.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename Parameters>
static void BindDirectLightReservoirSurface(
    FrameGraphBuilder& builder,
    Parameters& parameters,
    const RenderFrameGraphResources& resources)
{
	const GBufferRenderTargets& gbuffer = resources.Transient.GBuffer;
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferNormal = builder.CreateSRV(gbuffer.Normal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	BindSceneShaderParameters(builder, parameters, resources);
}

void AddDirectLightReservoirTemporalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& parameters = builder.AllocParameters<DirectLightReservoirTemporalCS>();
	parameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	parameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Previous);
	parameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Previous);
	parameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.ReservoirHistory.Surface.Previous);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);

	BindDirectLightReservoirSurface(builder, parameters, resources);

	const auto invalidateTemporalHistory = [](auto& fields, bool hasBeenProduced)
	{
		if (!hasBeenProduced)
		{
			ViewTemporalUniformData temporal = *fields.ViewTemporal.GetValue();
			temporal.HistoryValid = 0u;
			fields.ViewTemporal = temporal;
		}
	};

	builder.AddResourceProductionSetup(parameters, shadowSignals.ReservoirHistory.Sample.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(parameters, shadowSignals.ReservoirHistory.Weight.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(parameters, shadowSignals.ReservoirHistory.Surface.Previous, invalidateTemporalHistory);

	builder.Dispatch<DirectLightReservoirTemporalCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}

void AddDirectLightReservoirSpatialPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& parameters = builder.AllocParameters<DirectLightReservoirSpatialCS>();
	parameters->TemporalReservoirSample = builder.CreateSRV(shadowSignals.TemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateSRV(shadowSignals.TemporalReservoirWeight);
	parameters->CurrentReservoirSample = builder.CreateUAV(shadowSignals.ReservoirHistory.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateUAV(shadowSignals.ReservoirHistory.Weight.Current);
	parameters->CurrentReservoirSurface = builder.CreateUAV(shadowSignals.ReservoirHistory.Surface.Current);

	BindDirectLightReservoirSurface(builder, parameters, resources);

	builder.Dispatch<DirectLightReservoirSpatialCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
