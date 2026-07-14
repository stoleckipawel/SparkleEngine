#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLightingAccumulation.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/ReferenceLightingAccumulationPass.h"

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocParameters<ReferenceLightingAccumulationPass::Parameters>();
	parameters->ReferenceLightingSample = builder.CreateSRV(referenceSample);
	parameters->SceneColorTexture = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	parameters->PreviousReferenceLighting = builder.CreateSRV(resources.History.ReferenceLighting.Previous);
	parameters->CurrentReferenceLighting = builder.CreateUAV(resources.History.ReferenceLighting.Current);
	parameters->ReferenceSampleValidity = builder.CreateSRV(resources.Transient.Lighting.IndirectDiffuse);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);
	builder.Dispatch<ReferenceLightingAccumulationPass>(parameters);
}
