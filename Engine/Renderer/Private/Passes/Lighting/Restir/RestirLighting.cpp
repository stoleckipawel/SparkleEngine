#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirLighting.h"

#include "Passes/Lighting/Restir/RestirDirectLighting.h"
#include "Passes/Lighting/Restir/RestirIndirectLighting.h"

void AddRestirLightingProducerPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources)
{
	AddRestirDirectLightingPasses(builder, sceneExtent, resources);
	AddRestirIndirectLightingPasses(builder, sceneExtent, resources);
}
