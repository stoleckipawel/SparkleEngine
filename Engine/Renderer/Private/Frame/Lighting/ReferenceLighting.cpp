#include "../../PCH.h"
#include "Frame/Lighting/ReferenceLighting.h"

#include "Frame/Lighting/PathTracedDirectLighting.h"
#include "Frame/Lighting/PathTracedIndirectLighting.h"
#include "Frame/Lighting/ReferenceLightingAccumulation.h"

void AddReferenceLightingProducerPasses(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources)
{
	AddPathTracedDirectLightingPass(builder, resources);
	AddPathTracedIndirectLightingPass(builder, resources);
}

void FinalizeReferenceLightingPasses(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSample,
    const FrameAssemblyResourceLayout& resources)
{
	AddReferenceLightingAccumulationPass(builder, referenceSample, resources);
}
