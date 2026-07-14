#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/Debug/Debug.h"
#include "Frame/PostProcessing/Exposure.h"
#include "Frame/Presentation/Presentation.h"
#include "Frame/Presentation/Upscaling.h"

void AddPreReconstructionPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    FrameAssemblyResourceLayout& resources)
{
	AddExposurePass(
	    builder,
	    renderExtent,
	    resources.Transient.Scene.SceneColor,
	    resources.History.Exposure,
	    resources.Transient.Exposure);
}

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer,
    FrameAssemblyResourceLayout& resources)
{
	if (!resources.FinalSceneColorProduced)
	{
		AddUpscalingPasses(builder, renderExtent, outputExtent, resources);
	}

	AddDebugPasses(builder, resources);

	if (presentToBackBuffer)
	{
		AddPresentationPass(builder, outputExtent, backBufferFormat, resources.Transient.Scene, resources.Transient.Exposure);
	}
}
