#include "PCH.h"

#include "Frame/Builders/FrameContextBuilder.h"

#include "Frame/Core/FrameContext.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "Scene/Preparation/RenderScenePreparation.h"
#include "Scene/RenderScene.h"
#include "SceneData/GpuScene/PersistentRenderGpuScene.h"
#include "View/RenderViewBuilder.h"
#include "View/RenderViewPreparation.h"
#include "View/RenderViewState.h"

FrameContextBuilder::FrameContextBuilder(
    RenderScene& scene,
    RenderScenePreparation& renderScenePreparation,
    RenderViewBuilder& renderViewBuilder,
    RenderViewPreparation& renderViewPreparation) noexcept :
    m_scene(scene),
    m_renderScenePreparation(renderScenePreparation),
    m_renderViewBuilder(renderViewBuilder),
    m_renderViewPreparation(renderViewPreparation)
{
}

void FrameContextBuilder::Build(FrameContext& frame, const FrameContextBuildRequest& request) const
{
	m_renderScenePreparation.Execute(m_scene, frame.preparedScene);
	m_renderViewBuilder.Build(
	    frame.view,
	    request.ViewState,
	    RenderViewBuildRequest{
	        .Input = request.ViewInput,
	        .ViewportRequest = request.ViewportRequest,
	        .RenderExtent = request.RenderExtent,
	        .OutputExtent = request.OutputExtent,
	        .ViewMode = request.ViewMode,
	        .FrameId = request.FrameId,
	        .SceneGeneration = m_scene.GetSceneGeneration(),
	        .ShaderGeneration = request.ShaderGeneration,
	        .ImageProviderGeneration = request.ImageProviderGeneration,
	        .GraphTopologyGeneration = request.GraphTopologyGeneration});
			
	m_renderViewPreparation.Prepare(frame.preparedScene, frame.view, request.RayTracingScene);

	frame.sceneGpuData = &request.GpuScene.Update(frame.preparedScene, frame.view, request.FrameIndex);
}
