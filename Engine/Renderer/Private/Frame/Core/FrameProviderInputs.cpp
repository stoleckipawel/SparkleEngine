#include "../../PCH.h"
#include "Frame/Core/FrameProviderInputs.h"

FrameUpscalerProviderResources BuildFrameUpscalerProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle exposure)
{
	return FrameUpscalerProviderResources{
	    .ScalingInputColor = sceneTargets.SceneColor,
	    .ScalingOutputColor = sceneTargets.FinalSceneColor,
	    .Depth = sceneTargets.MainDepth,
	    .MotionVectors = gbuffer.MotionVector,
	    .Exposure = exposure};
}
