#pragma once

class FrameExecutionDiagnostics;
class FrameGraph;
class RenderDeviceServices;
class TaskExecutor;
struct PreparedRenderScene;
struct RenderFrameIdentity;
struct RenderFrameGraphResources;
struct RenderFrameTime;
struct RenderRayTracingFrameBindings;
struct RenderView;

namespace RenderFrameGraphExecution
{
	void Execute(
	    FrameGraph& frameGraph,
	    const RenderFrameGraphResources& resources,
	    const RenderFrameIdentity& identity,
	    const RenderFrameTime& time,
	    const PreparedRenderScene& scene,
	    const RenderView& view,
	    const RenderRayTracingFrameBindings& rayTracingBindings,
	    RenderDeviceServices& deviceServices,
	    FrameExecutionDiagnostics& diagnostics,
	    TaskExecutor& taskExecutor);
}
