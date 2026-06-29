#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class FrameGraphBuilder;

namespace ExposureMomentChain
{
	struct Texture final
	{
		FrameGraphTextureHandle TextureHandle = FrameGraphTextureHandle::Invalid();
		std::uint32_t Width = 1u;
		std::uint32_t Height = 1u;
	};

	Texture AddReduction(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameGraphTextureHandle sceneColor);
	Texture AddDownsample(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent, FrameGraphTextureHandle sceneColor);
}
