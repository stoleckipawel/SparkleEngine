#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

LightingRenderTargets CreateLightingRenderTargets(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat radianceFormat,
    bool createRayReconstructionGuides);
