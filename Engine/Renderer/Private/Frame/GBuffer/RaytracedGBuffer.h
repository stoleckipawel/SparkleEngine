#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;

void AddRaytracedGBufferPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas);
