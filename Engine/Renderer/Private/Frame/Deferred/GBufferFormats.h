#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

namespace GBufferFormats
{
	inline constexpr PixelFormat BaseColor = PixelFormat::R8G8B8A8_UNorm;
	inline constexpr PixelFormat Normal = PixelFormat::R16G16B16A16_Float;
	inline constexpr PixelFormat Material = PixelFormat::R8G8B8A8_UNorm;
	inline constexpr PixelFormat Emissive = PixelFormat::R16G16B16A16_Float;
	inline constexpr PixelFormat Subsurface = PixelFormat::R8G8B8A8_UNorm;
	inline constexpr PixelFormat DeviceZ = PixelFormat::R32_Float;
	inline constexpr PixelFormat MotionVector = PixelFormat::R16G16_Float;
}
