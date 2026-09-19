#include "PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerProducts.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"

void PublishReferencePathTracerProducts(
    const ReferencePathTracerGraphResources& graphResources,
    RenderFrameGraphResources& resources)
{
	resources.ViewportProducts.Radiance = graphResources.CommittedMean;
	resources.ViewportProducts.RadianceSecondMoment = graphResources.CommittedM2;
	resources.ViewportProducts.SceneDepth = FrameGraphTextureHandle::Invalid();
}
