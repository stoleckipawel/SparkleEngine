#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;

LightingRenderTargets CreateLightingRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, PixelFormat radianceFormat);
