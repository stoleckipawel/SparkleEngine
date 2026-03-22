#pragma once

#include "RHI/Public/RenderConfig.h"
#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/Frame/RenderViewContext.h"
#include "Renderer/Public/SceneData/RenderSceneData.h"

#include <array>
#include <cstddef>

struct SPARKLE_RENDERER_API FrameContext
{
	static constexpr std::size_t MaxShadowViews = RenderConfig::Shadows::MaxShadowMaps;

	RenderSceneData sceneData = {};
	RenderViewContext mainView = {};
	std::array<RenderViewContext, MaxShadowViews> shadowViews = {};
	std::size_t shadowViewCount = 0;
};
