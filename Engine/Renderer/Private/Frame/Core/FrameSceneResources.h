#pragma once

#include "Frame/Core/Frame.h"

class FrameGraph;
class FrameGraphBuilder;
struct RenderSceneGpuBindings;

void CreateFrameSceneResources(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources);
RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder);
void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuBindings& sceneGpuBindings) noexcept;
