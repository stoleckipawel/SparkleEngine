#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

enum class RenderViewPresentationDomain : std::uint8_t
{
	SceneReferredHdr,
	DisplayLinearExact,
};

enum class SceneUpscalingMethod : std::uint8_t
{
	ConfiguredProvider,
	Linear,
	Point,
};

RenderViewPresentationDomain ResolveRenderViewPresentationDomain(RenderViewMode viewMode);
SceneUpscalingMethod ResolveSceneUpscalingMethod(RenderViewMode viewMode);
RenderViewportExtent ResolveSceneRenderExtent(
    RenderViewMode viewMode,
    RenderViewportExtent outputExtent,
    RenderViewportExtent configuredRenderExtent);
