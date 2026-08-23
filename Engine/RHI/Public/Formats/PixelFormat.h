#pragma once

#include "../RHIAPI.h"

#include <cstdint>

enum class PixelFormat : std::uint16_t
{
	Unknown = 0,
	R32G32B32A32_Float = 2,
	R16G16B16A16_Float = 10,
	R8G8B8A8_UNorm = 28,
	R8G8B8A8_UNorm_Srgb = 29,
	R16G16_Float = 34,
	D32_Float = 40,
	R32_Float = 41,
	D24_UNorm_S8_UInt = 45,
	BC1_UNorm = 71,
	BC1_UNorm_Srgb = 72,
	BC2_UNorm = 74,
	BC2_UNorm_Srgb = 75,
	BC3_UNorm = 77,
	BC3_UNorm_Srgb = 78,
	BC4_UNorm = 80,
	BC4_SNorm = 81,
	BC5_UNorm = 83,
	BC5_SNorm = 84,
	B8G8R8A8_UNorm = 87,
	B8G8R8A8_UNorm_Srgb = 91,
	BC6H_UF16 = 95,
	BC7_UNorm = 98,
	BC7_UNorm_Srgb = 99,
};

SPARKLE_RHI_API const char* PixelFormatName(PixelFormat format) noexcept;
SPARKLE_RHI_API bool IsColorAttachmentPixelFormat(PixelFormat format) noexcept;
SPARKLE_RHI_API bool IsDepthStencilPixelFormat(PixelFormat format) noexcept;
SPARKLE_RHI_API bool PixelFormatHasStencilAspect(PixelFormat format) noexcept;
SPARKLE_RHI_API bool IsSrgbPixelFormat(PixelFormat format) noexcept;
SPARKLE_RHI_API PixelFormat PixelFormatToLinear(PixelFormat format) noexcept;
SPARKLE_RHI_API std::uint32_t PixelFormatBytesPerTexel(PixelFormat format) noexcept;
SPARKLE_RHI_API PixelFormat PixelFormatFromSerializedTextureFormat(std::uint32_t value) noexcept;
SPARKLE_RHI_API std::uint32_t PixelFormatToSerializedTextureFormat(PixelFormat format) noexcept;
