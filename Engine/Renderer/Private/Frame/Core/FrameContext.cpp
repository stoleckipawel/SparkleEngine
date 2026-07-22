#include "PCH.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Core/FrameContext.h"

#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Temporal/TemporalFrameState.h"
#include "Camera/RenderCamera.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/RenderWorld.h"
#include "SceneData/RenderSceneGpuData.h"

#include <cstdio>
#include <utility>

namespace
{
	RhiViewport BuildSceneViewport(RenderViewportExtent sceneExtent) noexcept
	{
		return RhiViewport{
		    .X = 0.0f,
		    .Y = 0.0f,
		    .Width = static_cast<float>(sceneExtent.Width),
		    .Height = static_cast<float>(sceneExtent.Height),
		    .MinDepth = 0.0f,
		    .MaxDepth = 1.0f};
	}

	RhiRect BuildSceneScissorRect(RenderViewportExtent sceneExtent) noexcept
	{
		return RhiRect{
		    .Left = 0,
		    .Top = 0,
		    .Right = static_cast<std::int32_t>(sceneExtent.Width),
		    .Bottom = static_cast<std::int32_t>(sceneExtent.Height)};
	}
}

FrameContext BuildFrameContext(
    const RenderWorld& world,
    const RenderFrameDynamicData& dynamic,
    RhiResourceService& resourceService,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    RenderRayTracingScene* renderRayTracingScene,
    PerViewDataBuilder& perViewDataBuilder,
    TemporalDataBuilder& temporalDataBuilder)
{
	FrameContext frame{};
	frame.sceneData = renderSceneDataBuilder.Build(world, dynamic);
	const PerViewCameraConstantBufferData cameraData = renderCamera.GetCameraConstantBufferData();
	if (renderRayTracingScene != nullptr)
	{
		renderRayTracingScene->PlanFrame(frame.sceneData, cameraData.Position);
	}
	frame.sceneGpuData = BuildRenderSceneGpuData(resourceService, frame.sceneData);
	const RhiViewport sceneViewport = BuildSceneViewport(sceneExtent);
	frame.mainView = perViewDataBuilder.BuildView(cameraData, sceneViewport, BuildSceneScissorRect(sceneExtent));
	frame.mainView.perTemporalData = temporalDataBuilder.BuildTemporalData(
	    renderCamera,
	    frame.mainView.perViewData.Camera,
	    sceneViewport,
	    dynamic.Metadata.FrameId);
	frame.mainView.temporalState = BuildRenderTemporalFrameState(frame.mainView.perTemporalData);

	return frame;
}
