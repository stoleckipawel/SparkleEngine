#include "../../PCH.h"
#include "Frame/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/PostProcessing/PostProcessingPasses.h"
#include "Passes/RayTracing/RayTracingScenePass.h"
#include "Passes/Scene/SceneRenderingPasses.h"
#include "Scene/RenderScene.h"

RenderFrameGraphResources FramePipeline::BuildRenderFrameGraph(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings)
{
	RenderRayTracingScene& rayTracingScene = m_renderScene->GetRayTracingScene();

	RenderFrameGraphResources resources = {};
	CreateRenderFrameGraphResources(builder, settings, resources);
	AddRayTracingScenePass(builder, rayTracingScene, resources);

	AddSceneRenderingPasses(
	    builder,
	    settings,
	    m_viewportRenderRequest.ViewMode,
	    rayTracingScene,
	    *m_gpuMeshCache,
	    *m_imageProviders,
	    *m_referencePathTracerSession,
	    resources);
	AddPostProcessingPasses(builder, settings, resources);

	return resources;
}
