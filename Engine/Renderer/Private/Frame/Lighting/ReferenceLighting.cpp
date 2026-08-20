#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLighting.h"

#include "Frame/Lighting/PathTracedDirectLighting.h"
#include "Frame/Lighting/PathTracedIndirectLighting.h"
#include "Frame/Lighting/ReferenceLightingAccumulation.h"

void AddReferenceLightingProducerPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameAssemblyResourceLayout& resources)
{
	AddPathTracedDirectLightingPass(builder, sceneExtent, resources);
	AddPathTracedIndirectLightingPass(builder, sceneExtent, resources);
}

void FinalizeReferenceLightingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources)
{
	AddReferenceLightingAccumulationPass(builder, sceneExtent, referenceSample, resources);
}
