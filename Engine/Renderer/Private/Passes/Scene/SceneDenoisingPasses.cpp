#include "../../PCH.h"
#include "Passes/Scene/SceneDenoisingPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstruction.h"

void AddSceneDenoisingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	AddRestirRayReconstructionPass(builder, settings, imageProviders, resources);
}
