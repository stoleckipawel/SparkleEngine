#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

namespace FrameRenderFormats
{
	inline constexpr PixelFormat SceneColor = PixelFormat::R32G32B32A32_Float;
	inline constexpr PixelFormat DepthStencil = PixelFormat::D24_UNorm_S8_UInt;
}
