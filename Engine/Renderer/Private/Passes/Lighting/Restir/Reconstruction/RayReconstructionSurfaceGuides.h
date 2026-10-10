#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrame;
struct RenderFrameGraphResources;

void AddRayReconstructionSurfaceGuidesPass(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent sceneExtent, RenderFrameGraphResources& resources);
