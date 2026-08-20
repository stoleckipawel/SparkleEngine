#pragma once

class FrameGraphBuilder;
class IRayReconstructionProvider;
struct FrameAssemblyResourceLayout;
struct RenderViewportExtent;

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    IRayReconstructionProvider* rayReconstructionProvider,
    FrameAssemblyResourceLayout& resources);
