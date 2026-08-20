#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;
class GpuMeshCache;
struct FrameAssemblyExternalResources;

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const GBufferRenderTargets& targets,
    const FrameAssemblyExternalResources& externalResources);
