#include "../../PCH.h"
#include "Frame/FramePipeline.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/PresentationPasses.h"
#include "Passes/Scene/RayTracingScenePass.h"
#include "Passes/Scene/SceneRenderingPasses.h"
#include "Scene/RenderScene.h"

RenderFrameGraphResources FramePipeline::BuildRenderFrameGraph(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings)
{
	RenderRayTracingScene& rayTracingScene = m_renderScene->GetRayTracingScene();

	RenderFrameGraphResources resources = CreateRenderFrameGraphResources(builder, settings);
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

	AddPresentationPasses(builder, settings, m_viewportRenderRequest.ViewMode, resources);

	return resources;
}
