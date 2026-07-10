#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLightingAccumulation.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/ReferenceLightingAccumulationPass.h"

void AddReferenceLightingAccumulationPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources)
{
	auto& parameters = builder.AllocPassParameters<ReferenceLightingAccumulationPass>();
	ReferenceLightingAccumulationPass::DeclareResources(
	    builder,
	    referenceSample,
	    resources.Transient.Scene.SceneColor,
	    resources.History.PreviousReferenceLighting,
	    resources.History.CurrentReferenceLighting,
	    resources.Transient.Lighting.IndirectDiffuse,
	    resources.Transient.GBuffer.MotionVector,
	    parameters);
	builder.AddComputeShaderPass<ReferenceLightingAccumulationPass>(parameters);
}
