#include "../../../PCH.h"
#include "Passes/Lighting/Reference/ReferenceLightingAccumulation.h"

#include "Core/Public/Math/MathUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/ReferenceLightingAccumulationShader.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<ReferenceLightingAccumulationCS>();
	parameters->ReferenceLightingSample = builder.CreateSRV(referenceSample);
	parameters->SceneColorTexture = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->PreviousReferenceLighting = builder.CreateSRV(resources.History.ReferenceLighting.Previous);
	parameters->CurrentReferenceLighting = builder.CreateUAV(resources.History.ReferenceLighting.Current);
	parameters->ReferenceSampleValidity = builder.CreateSRV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);
	builder.AddPassParameterSetup(
	    parameters,
	    [](auto& fields)
	    {
		    fields.ReferenceLightingAccumulationConstants = ReferenceLightingAccumulationUniformData{
		        .SamplesFrame = BuildPathTracedLightingSettings().SamplesPerPixel,
		        .HistoryValid = 0u};
	    });
	builder.AddResourceProductionSetup(
	    parameters,
	    resources.History.ReferenceLighting.Previous,
	    [](auto& fields, bool hasBeenProduced)
	    {
		    ReferenceLightingAccumulationUniformData constants = *fields.ReferenceLightingAccumulationConstants.GetValue();
		    constants.HistoryValid = hasBeenProduced ? 1u : 0u;
		    fields.ReferenceLightingAccumulationConstants = constants;
	    });
	builder.Dispatch<ReferenceLightingAccumulationCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
