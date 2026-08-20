#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLightingAccumulation.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/ReferenceLightingAccumulationPass.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingSettings.h"

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<ReferenceLightingAccumulationPass::Parameters>();
	auto* parameterFields = parameters.operator->();
	parameters->ReferenceLightingSample = builder.CreateSRV(referenceSample);
	parameters->SceneColorTexture = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->PreviousReferenceLighting = builder.CreateSRV(resources.History.ReferenceLighting.Previous);
	parameters->CurrentReferenceLighting = builder.CreateUAV(resources.History.ReferenceLighting.Current);
	parameters->ReferenceSampleValidity = builder.CreateSRV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);
	builder.AddPassParameterSetup(
	    [parameterFields]
	    {
		    parameterFields->ReferenceLightingAccumulationConstants = ReferenceLightingAccumulationUniformData{
		        .SamplesFrame = BuildPathTracedLightingSettings().SamplesPerPixel,
		        .HistoryValid = 0u};
	    });
	builder.AddReferenceLightingHistorySetup(
	    [parameterFields](bool historyValid)
	    {
		    ReferenceLightingAccumulationUniformData constants = *parameterFields->ReferenceLightingAccumulationConstants.GetValue();
		    constants.HistoryValid = historyValid ? 1u : 0u;
		    parameterFields->ReferenceLightingAccumulationConstants = constants;
	    });
	builder.Dispatch<ReferenceLightingAccumulationPass>(parameters, sceneExtent.Width, sceneExtent.Height);
}
