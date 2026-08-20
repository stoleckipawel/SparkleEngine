#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;
struct FrameAssemblyExternalResources;

void AddRaytracedGBufferPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const FrameAssemblyExternalResources& externalResources);
