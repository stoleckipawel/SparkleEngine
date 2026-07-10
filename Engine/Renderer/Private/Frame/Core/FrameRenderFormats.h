#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

namespace FrameRenderFormats
{
	inline constexpr PixelFormat SceneColor = PixelFormat::R16G16B16A16_Float;
	inline constexpr PixelFormat SceneDepth = PixelFormat::R32_Float;
}
