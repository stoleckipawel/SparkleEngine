#include "../../PCH.h"
#include "Frame/Graph/ExecuteRenderFrameGraph.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/RenderFrame.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RenderSceneFrameGraphBindings.h"
#include "ShaderData/FrameUniformData.h"
#include "View/RenderView.h"

static FrameUniformData BuildFrameUniformData(std::uint64_t frameId, const RenderFrameTime& time) noexcept
{
	return FrameUniformData{
	    .FrameIndex = static_cast<std::uint32_t>(frameId),
	    .TotalTimeSeconds = static_cast<float>(time.UnscaledTime.count()),
	    .DeltaTimeSeconds = static_cast<float>(time.UnscaledDelta.count()),
	    .ScaledTotalTimeSeconds = static_cast<float>(time.ScaledTime.count()),
	    .ScaledDeltaTimeSeconds = static_cast<float>(time.ScaledDelta.count())};
}

void ExecuteRenderFrameGraph(
    FrameGraph& frameGraph,
    const RenderFrameGraphResources& resources,
    const RenderFrame& frame,
    RenderDeviceServices& deviceServices,
    FrameExecutionDiagnostics& diagnostics,
    TaskExecutor& taskExecutor)
{
	const PreparedRenderScene& scene = frame.PreparedScene;
	const RenderView& view = frame.View;

	BindRenderSceneFrameGraphResources(frameGraph, resources, scene, frame.RayTracingBindings);

	frameGraph.ApplyPassParameterDefaults();
	frameGraph.ApplyParameters(BuildFrameUniformData(frame.Identity.FrameId, frame.Time));
	frameGraph.ApplyParameters(scene);
	frameGraph.ApplyParameters(view);

	frameGraph.Setup();
	const FrameGraphPlan& plan = frameGraph.Compile();
	frameGraph.PreparePasses();
	frameGraph.ApplyResourceProductionSetups();
	frameGraph.Execute(plan, deviceServices, diagnostics, taskExecutor);
}
