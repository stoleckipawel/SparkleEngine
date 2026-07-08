#pragma once

#include "Frame/Core/FrameRenderPath.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

struct FrameResolutionExtents final
{
	RenderViewportExtent Render;
	RenderViewportExtent Output;
};

FrameResolutionExtents ResolveFrameResolutionExtents(
    RenderViewportExtent outputExtent,
    FrameRenderPath renderPath) noexcept;
