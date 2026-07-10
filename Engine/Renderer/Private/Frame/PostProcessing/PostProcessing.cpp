#include "../../PCH.h"
#include "Frame/PostProcessing/PostProcessing.h"

#include "Frame/Debug/Debug.h"
#include "Frame/PostProcessing/Exposure.h"
#include "Frame/Presentation/Presentation.h"
#include "Frame/Presentation/Upscaling.h"
#include "RayReconstruction/RayReconstructionFramePass.h"

void AddPostProcessingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer,
    FrameAssemblyResourceLayout& resources)
{
	AddExposurePass(
	    builder,
	    renderExtent,
	    resources.Transient.Scene.SceneColor,
	    resources.History.PreviousExposure,
	    resources.History.CurrentExposure,
	    resources.Transient.Exposure);

	resources.ReconstructedSceneColorProduced =
	    TryAddRayReconstructionProviderPass(builder, "RayReconstruction", renderExtent, resources.RayReconstructionProviderInputs);

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
