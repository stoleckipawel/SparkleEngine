#pragma once

class FrameGraph;
struct PreparedRenderScene;
struct RenderFrameGraphResources;
struct RenderRayTracingFrameBindings;

void BindRenderSceneFrameGraphResources(
    FrameGraph& frameGraph,
    const RenderFrameGraphResources& resources,
    const PreparedRenderScene& scene,
    const RenderRayTracingFrameBindings& rayTracingBindings);
