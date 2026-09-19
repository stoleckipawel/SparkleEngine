#include "../../PCH.h"
#include "Passes/Presentation/SceneUpscaling.h"

#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/LinearUpscaling.h"
#include "Providers/RendererImageProviderStack.h"
#include "Upscaling/UpscalerPass.h"
#include "Upscaling/UpscalerSettings.h"

void AddSceneUpscalingPass(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RendererImageProviderStack& imageProviders,
    RenderFrameGraphResources& resources)
{
	if (resources.ResolvedSceneColor.IsValid())
	{
		return;
	}

	resources.ResolvedSceneColor = CreateResolvedSceneColor(builder, settings.OutputExtent);
	const UpscalerPassResources inputs{
	    .InputColor = resources.Transient.Scene.SceneColor,
	    .OutputColor = resources.ResolvedSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure};

	AddLinearUpscalePass(builder, inputs.InputColor, inputs.OutputColor, settings.OutputExtent);
	IUpscalerProvider* upscalerProvider = settings.ImagePipeline == ImageProviderPipeline::NativeResolution
	    ? nullptr
	    : imageProviders.GetUpscalerProvider();
	if (IsExternalUpscalerEnabled() && upscalerProvider != nullptr)
	{
		AddUpscalerPass(builder, *upscalerProvider, settings.RenderExtent, settings.OutputExtent, inputs);
	}
}
