#pragma once

class FrameGraphBuilder;
class GpuMeshCache;
struct RenderFrameGraphResources;

void AddRasterizedGBufferMeshPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const RenderFrameGraphResources& resources);
