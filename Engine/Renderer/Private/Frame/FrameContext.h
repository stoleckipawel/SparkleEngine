#pragma once

#include "Config/RenderConfig.h"
#include "Renderer/Public/RendererAPI.h"
#include "Frame/RenderViewContext.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <array>
#include <cstddef>

struct SPARKLE_RENDERER_API FrameContext
{
	static constexpr std::size_t MaxShadowViews = RenderConfig::Shadows::MaxShadowMaps;

	RenderSceneData sceneData = {};
	RenderViewContext mainView = {};
	std::array<RenderViewContext, MaxShadowViews> shadowViews = {};
	std::size_t shadowViewCount = 0;
	RhiGpuVirtualAddress rayTracingSceneGpuAddress = 0;
};
