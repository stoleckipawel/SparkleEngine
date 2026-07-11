#include "../../PCH.h"
#include "Frame/Core/Frame.h"

#include "Frame/Core/FrameSceneResources.h"
#include "Frame/Deferred/GBuffer.h"
#include "Frame/Lighting/Lighting.h"
#include "Frame/PostProcessing/PostProcessing.h"
#include "Frame/RayTracing/RayTracingScene.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

FrameBuildResult BuildFrame(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer)
{
	FrameAssemblyResourceLayout resources = {};
	CreateFrameSceneResources(builder, renderExtent, outputExtent, backBufferFormat, resources);
	AddRaytracingScenePasses(builder, resources);
	AddGBufferPasses(builder, renderExtent, resources);
	AddLightingPasses(builder, renderExtent, resources);
	AddPreReconstructionPostProcessingPasses(builder, renderExtent, resources);
	AddLightingReconstructionPasses(builder, renderExtent, outputExtent, resources);

	AddPostProcessingPasses(builder, renderExtent, outputExtent, backBufferFormat, presentToBackBuffer, resources);

	return FrameBuildResult{.Resources = resources};
}
