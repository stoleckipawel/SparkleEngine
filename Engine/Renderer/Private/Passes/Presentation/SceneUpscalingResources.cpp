#include "../../PCH.h"
#include "Passes/Presentation/SceneUpscalingResources.h"

#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "Frame/Graph/RenderFrameGraphResources.h"

UpscalerPassResources CreateSceneUpscalingResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent outputExtent,
    RenderFrameGraphResources& resources)
{
	resources.ResolvedSceneColor = CreateResolvedSceneColor(builder, outputExtent);

	return UpscalerPassResources{
	    .InputColor = resources.Transient.Scene.SceneColor,
	    .OutputColor = resources.ResolvedSceneColor,
	    .Depth = resources.Transient.GBuffer.DeviceZ,
	    .MotionVectors = resources.Transient.GBuffer.MotionVector,
	    .Exposure = resources.Transient.Exposure};
}
