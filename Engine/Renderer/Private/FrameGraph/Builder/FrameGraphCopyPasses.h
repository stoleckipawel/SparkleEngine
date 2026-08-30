#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

#include <string_view>

class FrameGraphBuilder;

namespace FrameGraphCopyPasses
{
	void AddTextureCopy(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphTextureHandle destination,
	    FrameGraphTextureHandle source);
	void AddBufferCopy(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphBufferHandle destination,
	    FrameGraphBufferHandle source);
}
