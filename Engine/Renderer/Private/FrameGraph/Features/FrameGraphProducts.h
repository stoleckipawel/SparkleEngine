#pragma once

#include "Renderer/Public/FrameGraph/TextureHandle.h"
#include "Config/RenderConfig.h"

#include <array>
#include <cstddef>

struct FrameGraphSceneTargets
{
	TextureHandle SceneColor;
	TextureHandle BackBuffer;
	TextureHandle MainDepth;
};

struct FrameGraphGBufferTargets
{
	TextureHandle BaseColor;
	TextureHandle Normal;
	TextureHandle Material;
	TextureHandle Emissive;
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
