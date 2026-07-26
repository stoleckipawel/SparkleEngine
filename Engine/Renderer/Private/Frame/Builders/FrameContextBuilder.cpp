#include "PCH.h"
#include "Frame/Builders/FrameContextBuilder.h"
#include "Frame/Core/FrameContext.h"

#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Temporal/TemporalFrameState.h"
#include "Rendering/RenderFrameDynamicData.h"
#include "Camera/RenderCamera.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "SceneData/Preparation/RenderPreparationGraph.h"
#include "SceneData/GpuScene/PersistentRenderGpuScene.h"
#include "SceneData/RenderWorld.h"
#include "SceneData/RenderSceneGpuData.h"

RhiViewport FrameContextBuilder::BuildSceneViewport(
    RenderViewportExtent sceneExtent) noexcept
{
	return RhiViewport{
	    .X = 0.0f,
	    .Y = 0.0f,
	    .Width = static_cast<float>(sceneExtent.Width),
	    .Height = static_cast<float>(sceneExtent.Height),
	    .MinDepth = 0.0f,
	    .MaxDepth = 1.0f};
}

RhiRect FrameContextBuilder::BuildSceneScissorRect(
    RenderViewportExtent sceneExtent) noexcept
{
	return RhiRect{
	    .Left = 0,
	    .Top = 0,
	    .Right =
	        static_cast<std::int32_t>(sceneExtent.Width),
	    .Bottom =
	        static_cast<std::int32_t>(sceneExtent.Height)};
}

FrameContext FrameContextBuilder::Build(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    PersistentRenderGpuScene& gpuScene,
    std::uint32_t frameIndex,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderPreparationGraph& renderPreparationGraph,
    RenderRayTracingScene* renderRayTracingScene,
    PerViewDataBuilder& perViewDataBuilder,
    TemporalDataBuilder& temporalDataBuilder)
{
	FrameContext frame{};
	const PerViewCameraConstantBufferData cameraData = renderCamera.GetCameraConstantBufferData();
	frame.sceneData = renderPreparationGraph.Execute(
	    world,
	    dynamic,
	    renderCamera.GetFrustum());
	if (renderRayTracingScene != nullptr)
	{
		renderRayTracingScene->PlanFrame(frame.sceneData, cameraData.Position);
	}
	frame.sceneGpuData =
	    &gpuScene.Update(frame.sceneData, frameIndex);
	const RhiViewport sceneViewport =
	    BuildSceneViewport(sceneExtent);
	frame.mainView = perViewDataBuilder.BuildView(
	    cameraData,
	    sceneViewport,
	    BuildSceneScissorRect(sceneExtent));
	frame.mainView.perTemporalData = temporalDataBuilder.BuildTemporalData(
	    renderCamera,
	    frame.mainView.perViewData.Camera,
	    sceneViewport,
	    dynamic.Metadata.FrameId);
	frame.mainView.temporalState = BuildRenderTemporalFrameState(frame.mainView.perTemporalData);

	return frame;
}
