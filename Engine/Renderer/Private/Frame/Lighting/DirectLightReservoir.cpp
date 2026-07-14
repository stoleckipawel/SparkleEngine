#include "../../PCH.h"
#include "Frame/Lighting/DirectLightReservoir.h"

#include "Frame/Lighting/ShadowVisibility.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Deferred/DirectLightReservoirSpatialPass.h"
#include "Passes/Deferred/DirectLightReservoirTemporalPass.h"

void AddDirectLightReservoirPasses(
    FrameGraphBuilder& builder,
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
		parameters->DirectionalLights = builder.CreateSRV(externalResources.DirectionalLights);
		parameters->PointLights = builder.CreateSRV(externalResources.PointLights);
		parameters->SpotLights = builder.CreateSRV(externalResources.SpotLights);
		parameters->RectLights = builder.CreateSRV(externalResources.RectLights);
	};

	auto& temporalParameters = builder.AllocParameters<DirectLightReservoirTemporalPass::Parameters>();
	temporalParameters->TemporalReservoirSample = builder.CreateUAV(shadowSignals.TemporalReservoirSample);
	temporalParameters->TemporalReservoirWeight = builder.CreateUAV(shadowSignals.TemporalReservoirWeight);
	temporalParameters->PreviousReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Previous);
	temporalParameters->PreviousReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Previous);
	temporalParameters->PreviousReservoirSurface = builder.CreateSRV(shadowSignals.ReservoirHistory.Surface.Previous);
	temporalParameters->GBufferMotionVector = builder.CreateSRV(gbuffer.MotionVector);
	bindCommonParameters(temporalParameters);
	builder.Dispatch<DirectLightReservoirTemporalPass>(temporalParameters);

	auto& spatialParameters = builder.AllocParameters<DirectLightReservoirSpatialPass::Parameters>();
	spatialParameters->TemporalReservoirSample = builder.CreateSRV(shadowSignals.TemporalReservoirSample);
	spatialParameters->TemporalReservoirWeight = builder.CreateSRV(shadowSignals.TemporalReservoirWeight);
	spatialParameters->CurrentReservoirSample = builder.CreateUAV(shadowSignals.ReservoirHistory.Sample.Current);
	spatialParameters->CurrentReservoirWeight = builder.CreateUAV(shadowSignals.ReservoirHistory.Weight.Current);
	spatialParameters->CurrentReservoirSurface = builder.CreateUAV(shadowSignals.ReservoirHistory.Surface.Current);
	bindCommonParameters(spatialParameters);
	builder.Dispatch<DirectLightReservoirSpatialPass>(spatialParameters);
}
