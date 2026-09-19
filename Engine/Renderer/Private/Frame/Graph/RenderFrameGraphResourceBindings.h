#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraph;
class FrameGraphBuilder;
struct RenderSceneGpuBindings;

void CreateRenderFrameGraphResources(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources);
FrameGraphTextureHandle CreateResolvedSceneColor(FrameGraphBuilder& builder, RenderViewportExtent outputExtent);
RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder);
void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuBindings& sceneGpuBindings) noexcept;
