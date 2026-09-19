#include "../../PCH.h"
#include "Passes/Presentation/PresentationOutput.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "FrameGraph/Builder/FrameGraphCopyPasses.h"

void AddPresentationOutputPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    FrameGraphTextureHandle encodedColor,
    RenderFrameGraphResources& resources)
{
	if (settings.PresentationTarget == FramePresentationTarget::BackBuffer)
	{
		FrameGraphCopyPasses::AddTextureCopy(builder, "CopyEncodedColorToBackBuffer", resources.Presentation.BackBuffer, encodedColor);
	}

	resources.ViewportProducts.FinalColorLdr = encodedColor;
}
