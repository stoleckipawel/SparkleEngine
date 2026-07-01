#include "../../PCH.h"
#include "Frame/Core/FrameProviderInputs.h"

FrameAssemblyUpscalerProviderResources BuildFrameUpscalerProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    FrameGraphTextureHandle exposure)
{
	return FrameAssemblyUpscalerProviderResources{
	    .ScalingInputColor = sceneTargets.SceneColor,
	    .ScalingOutputColor = sceneTargets.FinalSceneColor,
	    .Depth = sceneTargets.MainDepth,
	    .MotionVectors = gbuffer.MotionVector,
	    .Exposure = exposure};
}

FrameAssemblyDenoiserProviderResources BuildFrameDenoiserProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle exposure)
{
	return FrameAssemblyDenoiserProviderResources{
	    .Depth = sceneTargets.MainDepth,
	    .MotionVectors = gbuffer.MotionVector,
	    .Exposure = exposure,
	    .Normals = gbuffer.Normal,
	    .IndirectReconstruction =
	        {.NoisyIndirectDiffuse = lighting.IndirectDiffuse,
	         .NoisyIndirectSpecular = lighting.IndirectSpecular}};
}
