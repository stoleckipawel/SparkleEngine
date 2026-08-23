#include "../../../PCH.h"
#include "Passes/Lighting/Reference/ReferenceLighting.h"

#include "Passes/Lighting/Direct/PathTracedDirectLighting.h"
#include "Passes/Lighting/PathTracedIndirectLighting.h"
#include "Passes/Lighting/Reference/ReferenceLightingAccumulation.h"

void AddReferenceLightingProducerPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	AddPathTracedDirectLightingPass(builder, sceneExtent, resources);
	AddPathTracedIndirectLightingPass(builder, sceneExtent, resources);
}

void FinalizeReferenceLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const RenderFrameGraphResources& resources)
{
	AddReferenceLightingAccumulationPass(builder, sceneExtent, referenceSample, resources);
}
