#pragma once

class FrameExecutionDiagnostics;
class FrameGraph;
class RenderDeviceServices;
class TaskExecutor;
struct RenderFrame;
struct RenderFrameGraphResources;

void ExecuteRenderFrameGraph(
    FrameGraph& frameGraph,
    const RenderFrameGraphResources& resources,
    const RenderFrame& frame,
    RenderDeviceServices& deviceServices,
    FrameExecutionDiagnostics& diagnostics,
    TaskExecutor& taskExecutor);
