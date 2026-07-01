#pragma once

#include "Frame/Reference/ReferenceRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

void AddReferencePathTracingPass(
    FrameGraphBuilder& builder,
    const ReferenceRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas);

void AddReferenceRenderingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);
