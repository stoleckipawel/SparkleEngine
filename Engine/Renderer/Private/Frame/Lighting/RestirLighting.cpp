#include "../../PCH.h"
#include "Frame/Lighting/RestirLighting.h"

#include "Frame/Lighting/RestirDirectLighting.h"
#include "Frame/Lighting/RestirIndirectLighting.h"
#include "Frame/Lighting/RestirRayReconstruction.h"

void AddRestirLightingProducerPasses(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameAssemblyResourceLayout& resources)
{
	AddRestirDirectLightingPasses(builder, sceneExtent, resources);
	AddRestirIndirectLightingPasses(builder, sceneExtent, resources);
}

void FinalizeRestirLightingPasses(FrameAssemblyResourceLayout& resources)
{
	ConfigureRestirRayReconstruction(resources);
}
