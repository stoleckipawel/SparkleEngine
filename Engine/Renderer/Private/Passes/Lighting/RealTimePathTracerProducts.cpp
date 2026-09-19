#include "../../PCH.h"
#include "Passes/Lighting/RealTimePathTracerProducts.h"

#include "Frame/Graph/RenderFrameGraphResources.h"

void PublishRealTimePathTracerProducts(RenderFrameGraphResources& resources)
{
	resources.ViewportProducts.Radiance = resources.Transient.Scene.SceneColor;
}
