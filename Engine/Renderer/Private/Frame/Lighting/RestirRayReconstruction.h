#pragma once

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;
struct RenderViewportExtent;

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    FrameAssemblyResourceLayout& resources);
