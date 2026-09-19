#include "../../PCH.h"
#include "Passes/Presentation/SceneUpscalingResources.h"

#include "Frame/Graph/RenderFrameGraphResources.h"

UpscalerPassResources CreateSceneUpscalingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    RenderFrameGraphResources& resources)
{
	resources.Presentation.ResolvedSceneColor = CreateResolvedSceneColorTarget(builder, outputExtent);

	return UpscalerPassResources{
	    .InputColor = resources.Presentation.SceneColorInput,
	    .OutputColor = resources.Presentation.ResolvedSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure};
}
