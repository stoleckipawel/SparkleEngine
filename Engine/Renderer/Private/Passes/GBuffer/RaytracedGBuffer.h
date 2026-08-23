#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;
struct RenderFrameGraphImportedSceneResources;

void AddRaytracedGBufferMeshPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const RenderFrameGraphImportedSceneResources& externalResources);
