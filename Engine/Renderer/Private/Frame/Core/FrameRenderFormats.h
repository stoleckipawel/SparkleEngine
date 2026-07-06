#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

namespace FrameRenderFormats
{
	inline constexpr PixelFormat SceneColor = PixelFormat::R16G16B16A16_Float;
	inline constexpr PixelFormat DepthStencil = PixelFormat::D24_UNorm_S8_UInt;
}
