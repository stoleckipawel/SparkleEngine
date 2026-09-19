#include "../../PCH.h"
#include "Passes/Presentation/SceneUpscalingPasses.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Presentation/LinearUpscale.h"
#include "Passes/Presentation/SceneUpscalingResources.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscale.h"
#include "Upscaling/UpscalerSettings.h"

void AddSceneUpscalingPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	if (resources.ResolvedSceneColor.IsValid())
	{
		return;
	}

	const UpscalerPassResources inputs = CreateSceneUpscalingResources(builder, settings.OutputExtent, resources);
	const EUpscalerProviderKind provider = CVarUpscalerProvider.Get();

	switch (provider)
	{
		case EUpscalerProviderKind::Linear:
			AddLinearUpscalePass(builder, inputs.InputColor, inputs.OutputColor, settings.OutputExtent);
			break;
		case EUpscalerProviderKind::NvidiaDlss:
			AddNvidiaDlssUpscalePass(builder, settings.RenderExtent, settings.OutputExtent, imageProviders, inputs);
			break;
		default:
			throw Diagnostics::Error("Scene upscaling graph construction received an invalid provider.");
	}
}
