#include "../../PCH.h"
#include "Frame/Core/Frame.h"

#include "Frame/Core/FrameSceneResources.h"
#include "Frame/Debug/Debug.h"
#include "Frame/Deferred/GBuffer.h"
#include "Frame/Lighting/Lighting.h"
#include "Frame/PostProcessing/PostProcessing.h"
#include "Frame/Presentation/Presentation.h"
#include "Frame/RayTracing/RayTracingScene.h"
#include "Frame/Reference/ReferencePathTracing.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

FrameBuildResult BuildFrame(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer)
{
	FrameAssemblyResourceLayout resources = {};

	CreateFrameSceneResources(builder, sceneExtent, backBufferFormat, resources);
	AddGBufferPasses(builder, sceneExtent, resources);
	AddRayTracingInfrastructurePasses(builder, resources);
	AddLightingPasses(builder, sceneExtent, resources);
	AddReferenceRenderingPasses(builder, resources);
	AddPostProcessingPasses(builder, sceneExtent, resources);
	AddDebugPasses(builder, resources);

	if (presentToBackBuffer)
	{
		AddPresentationPass(builder, sceneExtent, backBufferFormat, resources.Transient.Scene, resources.Transient.Exposure);
	}

	return FrameBuildResult{.Resources = resources};
}
