#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

#include <cstdint>

enum class RhiCapturePixelEncoding : std::uint8_t
{
	Rgba32Float,
	Rgba16Float,
	Rgba8Unorm,
	Bgra8Unorm,
};

struct RhiCaptureFormat final
{
	RhiCapturePixelEncoding Encoding = RhiCapturePixelEncoding::Rgba8Unorm;
	std::uint32_t BytesPerPixel = 0;
};

bool TryResolveRhiCaptureFormat(PixelFormat format, RhiCaptureFormat& captureFormat) noexcept;
