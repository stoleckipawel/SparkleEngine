#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

struct FrameResolutionExtents final
{
	RenderViewportExtent Render;
	RenderViewportExtent Output;
};

FrameResolutionExtents ResolveFrameResolutionExtents(RenderViewportExtent outputExtent) noexcept;
