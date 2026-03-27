#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "RHI/Public/RenderConfig.h"

#include <array>
#include <cstddef>

struct FrameGraphSceneTargets
{
	TextureHandle BackBuffer;
	TextureHandle MainDepth;
};

struct FrameGraphPresentationInputs
{
	TextureHandle BackBuffer;
};

struct FrameGraphShadowOutputs
{
	static constexpr std::size_t MaxShadowMaps = RenderConfig::Shadows::MaxShadowMaps;
	std::array<TextureHandle, MaxShadowMaps> ShadowMaps = {};
};

struct FrameGraphComputeShowcaseOutputs
{
	TextureHandle Color;
};
