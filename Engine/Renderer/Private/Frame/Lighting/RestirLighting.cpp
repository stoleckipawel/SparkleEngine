#include "../../PCH.h"
#include "Frame/Lighting/RestirLighting.h"

#include "Frame/Lighting/RestirDirectLighting.h"
#include "Frame/Lighting/RestirIndirectLighting.h"

void AddRestirLightingProducerPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	AddRestirDirectLightingPasses(builder, sceneExtent, resources);
	AddRestirIndirectLightingPasses(builder, sceneExtent, resources);
}
