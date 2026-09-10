#include "../../../PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracer.h"

#include "Passes/Lighting/Direct/ReferencePathTracerDirectLighting.h"
#include "Passes/Lighting/ReferencePathTracerIndirectLighting.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerAccumulation.h"

void AddReferencePathTracerProducerPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	AddReferencePathTracerDirectLightingPass(builder, sceneExtent, resources);
	AddReferencePathTracerIndirectLightingPass(builder, sceneExtent, resources);
}

void FinalizeReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameGraphTextureHandle referencePathTracerSample,
    const RenderFrameGraphResources& resources)
{
	AddReferencePathTracerAccumulationPass(builder, sceneExtent, referencePathTracerSample, resources);
}
