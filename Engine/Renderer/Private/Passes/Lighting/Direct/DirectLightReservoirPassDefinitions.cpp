#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPassDefinitions.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Direct/DirectLightReservoirSpatialShader.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "Passes/Lighting/Direct/DirectLightReservoirTemporalShader.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename Parameters> static void BindDirectLightReservoirSurface(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    Parameters& parameters,
    const RenderFrameGraphResources& resources)
{
	const GBufferRenderTargets& gbuffer = resources.Transient.GBuffer;
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(gbuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	BindSceneShaderParameters(builder, frame, parameters, resources);

	parameters->DirectLightingEvaluateDiffuse =
	    IsDirectDiffuseActive(gbuffer.BaseColor, resources.Transient.Lighting.DirectDiffuse) ? 1u : 0u;
	parameters->DirectLightingEvaluateSpecular =
	    IsDirectSpecularActive(gbuffer.Material, resources.Transient.Lighting.DirectSpecular) ? 1u : 0u;
	parameters->DirectLightingEvaluateSubsurface =
	    IsDirectSubsurfaceActive(gbuffer.Subsurface, resources.Transient.Lighting.DirectSubsurface) ? 1u : 0u;
	parameters->DirectLightingEvaluateShadows = IsDirectShadowsActive() ? 1u : 0u;
}

void AddDirectLightReservoirTemporalPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<DirectLightReservoirTemporalCS>();
	parameters->TemporalReservoirSample = builder.CreateUAV(resources.Transient.DirectLightTemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateUAV(resources.Transient.DirectLightTemporalReservoirWeight);
	parameters->PreviousReservoirSample = builder.CreateSRV(resources.History.DirectLightReservoir.Sample.Previous);
	parameters->PreviousReservoirWeight = builder.CreateSRV(resources.History.DirectLightReservoir.Weight.Previous);
	parameters->PreviousReservoirSurface = builder.CreateSRV(resources.History.DirectLightReservoir.Surface.Previous);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);

	BindDirectLightReservoirSurface(builder, frame, parameters, resources);

	if (!builder.IsTextureHistoryValid(resources.History.DirectLightReservoir.Sample)
	    || !builder.IsTextureHistoryValid(resources.History.DirectLightReservoir.Weight)
	    || !builder.IsTextureHistoryValid(resources.History.DirectLightReservoir.Surface))
	{
		ViewTemporalUniformData temporal = frame.View.temporalUniform;
		temporal.HistoryValid = 0u;
		parameters->ViewTemporal = temporal;
	}

	builder.Dispatch<DirectLightReservoirTemporalCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}

void AddDirectLightReservoirSpatialPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<DirectLightReservoirSpatialCS>();
	parameters->TemporalReservoirSample = builder.CreateSRV(resources.Transient.DirectLightTemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateSRV(resources.Transient.DirectLightTemporalReservoirWeight);
	parameters->CurrentReservoirSample = builder.CreateUAV(resources.History.DirectLightReservoir.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateUAV(resources.History.DirectLightReservoir.Weight.Current);
	parameters->CurrentReservoirSurface = builder.CreateUAV(resources.History.DirectLightReservoir.Surface.Current);

	BindDirectLightReservoirSurface(builder, frame, parameters, resources);

	builder.Dispatch<DirectLightReservoirSpatialCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
