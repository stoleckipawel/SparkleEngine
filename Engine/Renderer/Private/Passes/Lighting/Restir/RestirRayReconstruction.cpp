#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirRayReconstruction.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/Restir/RestirRayReconstructionResources.h"
#include "Providers/RendererImageProviderStack.h"
#include "RayReconstruction/RayReconstructionPass.h"

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	const RayReconstructionPassResources providerInputs = CreateRestirRayReconstructionResources(builder, settings.RenderExtent, resources);

	AddRayReconstructionPass(
	    builder,
	    *imageProviders.GetRayReconstructionProvider(),
	    "DlssRayReconstruction",
	    settings.RenderExtent,
	    settings.RenderExtent,
	    providerInputs);

	resources.Presentation.SceneColorInput = providerInputs.OutputColor;
}
