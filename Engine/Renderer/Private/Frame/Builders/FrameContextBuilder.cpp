#include "PCH.h"
#include "Frame/Builders/FrameContextBuilder.h"
#include "Frame/Core/FrameContext.h"

#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Temporal/TemporalFrameState.h"
#include "Camera/RenderCamera.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "SceneData/Preparation/RenderPreparationGraph.h"
#include "SceneData/GpuScene/PersistentRenderGpuScene.h"
#include "Scene/RenderScene.h"
#include "SceneData/RenderSceneGpuData.h"

FrameContextBuilder::FrameContextBuilder(
    RenderScene& scene,
    const RenderCamera& renderCamera,
    RenderPreparationGraph& renderPreparationGraph,
    PerViewDataBuilder& perViewDataBuilder,
    TemporalDataBuilder& temporalDataBuilder) noexcept :
    m_scene(scene),
    m_renderCamera(renderCamera),
    m_renderPreparationGraph(renderPreparationGraph),
    m_perViewDataBuilder(perViewDataBuilder),
    m_temporalDataBuilder(temporalDataBuilder)
{
}

RhiViewport FrameContextBuilder::BuildSceneViewport(RenderViewportExtent sceneExtent) noexcept
{
	return RhiViewport{
	    .X = 0.0f,
	    .Y = 0.0f,
	    .Width = static_cast<float>(sceneExtent.Width),
	    .Height = static_cast<float>(sceneExtent.Height),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f};
}

RhiRect FrameContextBuilder::BuildSceneScissorRect(RenderViewportExtent sceneExtent) noexcept
{
	return RhiRect{
	    .Left = 0,
	    .Top = 0,
	    .Right = static_cast<std::int32_t>(sceneExtent.Width),
	    .Bottom = static_cast<std::int32_t>(sceneExtent.Height)};
}

void FrameContextBuilder::Build(FrameContext& frame, const FrameContextBuildRequest& request) const
{
	const PerViewCameraConstantBufferData cameraData = m_renderCamera.GetCameraConstantBufferData();
	m_renderPreparationGraph.Execute(m_scene, m_renderCamera.GetFrustum(), cameraData.Position, frame.sceneData);

	if (request.RayTracingScene != nullptr)
	{
		request.RayTracingScene->PlanFrame(frame.sceneData, cameraData.Position);
	}

	frame.sceneGpuData = &request.GpuScene.Update(frame.sceneData, request.FrameIndex);

	const RhiViewport sceneViewport = BuildSceneViewport(request.SceneExtent);
	frame.mainView = m_perViewDataBuilder.BuildView(cameraData, sceneViewport, BuildSceneScissorRect(request.SceneExtent));
	frame.mainView.perTemporalData =
	    m_temporalDataBuilder.BuildTemporalData(m_renderCamera, frame.mainView.perViewData.Camera, sceneViewport, request.FrameId);
	frame.mainView.temporalState = BuildRenderTemporalFrameState(frame.mainView.perTemporalData);
}
