#include "PCH.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/FrameContext.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RenderViewLightingData.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Frame/TemporalFrameState.h"
#include "Camera/RenderCamera.h"
#include "Debug/RendererCVars.h"
#include "RayTracing/RayTracingPtlasPartitionPlanner.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"

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
    const RenderSceneSnapshot& sceneSnapshot,
    RenderHardwareInterface& renderHardwareInterface,
    const RenderCamera& renderCamera,
    RenderViewportExtent sceneExtent,
    RenderSceneDataBuilder& renderSceneDataBuilder,
    RayTracingPtlasPartitionPlanner& rayTracingPtlasPartitionPlanner,
    PerViewDataBuilder& perViewDataBuilder,
    ViewLightingBuilder& viewLightingBuilder,
    TemporalDataBuilder& temporalDataBuilder)
{
	FrameContext frame{};
	frame.sceneData = renderSceneDataBuilder.Build(sceneSnapshot);
	frame.rayTracingPtlasPartitionPlan = rayTracingPtlasPartitionPlanner.Build(
	    frame.sceneData,
	    RayTracingPtlasPartitionPlannerConfig{
	        .PartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get(),
	        .EnableGlobalPartition = CVarRayTracingGlobalPartition.Get()});
	frame.meshInstances = MeshInstanceFrameData::Build(renderHardwareInterface, frame.sceneData, &frame.rayTracingPtlasPartitionPlan);
	frame.skinning = SkinningFrameData::Build(renderHardwareInterface, frame.sceneData);
	const PerViewLightingConstantBufferData lighting = viewLightingBuilder.Build(frame.sceneData);
	const RhiViewport sceneViewport = BuildSceneViewport(sceneExtent);
	frame.mainView = perViewDataBuilder.BuildView(renderCamera.GetCameraConstantBufferData(), lighting, sceneViewport, BuildSceneScissorRect(sceneExtent));
	frame.mainView.perTemporalData = temporalDataBuilder.BuildTemporalData(renderCamera, frame.mainView.perViewData.Camera, sceneViewport);
	frame.mainView.temporalState = BuildRenderTemporalFrameState(frame.mainView.perTemporalData);

	frame.mainView.perViewGpuAddress = renderHardwareInterface.GetUploadService().AllocatePerViewConstantBuffer(frame.mainView.perViewData);

	return frame;
}
