#include "../../PCH.h"
#include "Frame/Presentation/Upscaling.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Presentation/LinearUpscaling.h"
#include "Upscaling/UpscalerPass.h"
#include "Upscaling/UpscalerSettings.h"

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    IUpscalerProvider* upscalerProvider,
    FrameAssemblyResourceLayout& resources)
{
	const UpscalerPassResources inputs{
	    .InputColor = resources.Transient.Scene.SceneColor,
	    .OutputColor = resources.Transient.Scene.FinalSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure};

	AddLinearUpscalePass(builder, inputs.InputColor, inputs.OutputColor, outputExtent);
	if (IsExternalUpscalerEnabled() && upscalerProvider != nullptr)
	{
		AddUpscalerPass(builder, *upscalerProvider, renderExtent, outputExtent, inputs);
	}

	resources.FinalSceneColorProduced = true;
}
