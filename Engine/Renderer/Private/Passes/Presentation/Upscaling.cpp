#include "../../PCH.h"
#include "Passes/Presentation/Upscaling.h"

#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/LinearUpscaling.h"
#include "Upscaling/UpscalerPass.h"
#include "Upscaling/UpscalerSettings.h"

void AddUpscalingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    IUpscalerProvider* upscalerProvider,
    RenderFrameGraphResources& resources)
{
	resources.ResolvedSceneColor = CreateResolvedSceneColor(builder, outputExtent);
	const UpscalerPassResources inputs{
	    .InputColor = resources.Transient.Scene.SceneColor,
	    .OutputColor = resources.ResolvedSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure};

	AddLinearUpscalePass(builder, inputs.InputColor, inputs.OutputColor, outputExtent);
	if (IsExternalUpscalerEnabled() && upscalerProvider != nullptr)
	{
		AddUpscalerPass(builder, *upscalerProvider, renderExtent, outputExtent, inputs);
	}
}
