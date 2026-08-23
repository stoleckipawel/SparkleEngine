#pragma once

class FrameGraphBuilder;
class IRayReconstructionProvider;
struct RenderFrameGraphResources;
struct RenderViewportExtent;

void AddRestirRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderViewportExtent outputExtent,
    IRayReconstructionProvider* rayReconstructionProvider,
    RenderFrameGraphResources& resources);
