#pragma once

#include "../Formats/PixelFormat.h"

#include <cstdint>

namespace RenderConfig
{
	inline constexpr unsigned FramesInFlight = 2u;

	inline constexpr PixelFormat BackBufferFormat = PixelFormat::R8G8B8A8_UNorm;

	inline constexpr int ShaderModelMajor = 6;
	inline constexpr int ShaderModelMinor = 0;

}  // namespace RenderConfig
