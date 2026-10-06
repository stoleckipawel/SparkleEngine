#pragma once

struct RenderFrame;

class FrameGraphBuilder;
class GpuMeshCache;
struct RenderFrameGraphResources;

void AddRasterizedGBufferMeshPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    GpuMeshCache& gpuMeshCache,
    const RenderFrameGraphResources& resources);
