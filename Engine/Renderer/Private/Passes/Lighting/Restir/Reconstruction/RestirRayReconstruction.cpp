#include "PCH.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstruction.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstructionResources.h"
#include "Providers/RendererImageProviderStack.h"
#include "RayReconstruction/RayReconstructionPass.h"

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	if (!settings.UseRayReconstruction)
	{
		return;
	}

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
