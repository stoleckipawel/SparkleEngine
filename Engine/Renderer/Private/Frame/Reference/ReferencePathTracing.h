#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

void AddReferencePathTracingPass(
    FrameGraphBuilder& builder,
    const LightingRenderTargets& lighting,
    FrameGraphAccelerationStructureHandle sceneTlas);

void AddReferenceRenderingPasses(FrameGraphBuilder& builder, FrameAssemblyResourceLayout& resources);
