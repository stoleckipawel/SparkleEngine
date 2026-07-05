#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

void AddReferencePathTracingPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle referenceSceneColor,
    FrameGraphAccelerationStructureHandle sceneTlas);

void AddReferenceRenderingPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources);
