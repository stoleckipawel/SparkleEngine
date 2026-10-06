#include "PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstruction.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Passes/Lighting/Restir/Reconstruction/RestirRayReconstructionResources.h"
#include "Providers/RendererImageProviderStack.h"
#include "RayReconstruction/RayReconstructionPass.h"

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	if (!IsRayReconstructionEnabled())
	{
		return;
	}

	const RayReconstructionPassResources providerInputs = CreateRestirRayReconstructionResources(builder, sceneExtent, resources);

	AddRayReconstructionPass(
	    builder,
	    *imageProviders.GetRayReconstructionProvider(),
	    "DlssRayReconstruction",
	    sceneExtent,
	    sceneExtent,
	    providerInputs);

	resources.Presentation.SceneColorInput = providerInputs.OutputColor;
}
