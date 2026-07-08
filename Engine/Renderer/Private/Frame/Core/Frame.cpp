#include "../../PCH.h"
#include "Frame/Core/Frame.h"

#include "Frame/Core/FrameRenderPath.h"
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
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    bool presentToBackBuffer)
{
	FrameAssemblyResourceLayout resources = {};
	const FrameRenderPath renderPath = ResolveFrameRenderPathFromSettings();

	CreateFrameSceneResources(builder, renderExtent, outputExtent, backBufferFormat, resources);
	AddRayTracingInfrastructurePasses(builder, resources);

	if (renderPath == FrameRenderPath::PathTracedReference)
	{
		AddReferenceRenderingPasses(builder, renderExtent, resources);
	}
	else
	{
		AddGBufferPasses(builder, renderExtent, resources);
		AddLightingPasses(builder, renderExtent, resources);
	}

	AddPostProcessingPasses(builder, renderExtent, outputExtent, resources);
	AddDebugPasses(builder, resources);

	if (presentToBackBuffer)
	{
		AddPresentationPass(builder, outputExtent, backBufferFormat, resources.Transient.Scene, resources.Transient.Exposure);
	}

	return FrameBuildResult{.Resources = resources, .RenderPath = renderPath};
}
