#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"

class FrameGraphBuilder;
class GpuMeshCache;
struct RenderFrameGraphImportedSceneResources;

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const GBufferRenderTargets& targets,
    const RenderFrameGraphImportedSceneResources& externalResources);
