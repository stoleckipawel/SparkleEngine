#include "PCH.h"

#include "Capture/RhiCaptureFormat.h"

bool TryResolveRhiCaptureFormat(PixelFormat format, RhiCaptureFormat& captureFormat) noexcept
{
	switch (format)
	{
		case PixelFormat::R32G32B32A32_Float:
			captureFormat = RhiCaptureFormat{.Encoding = RhiCapturePixelEncoding::Rgba32Float, .BytesPerPixel = 16u};
			return true;
		case PixelFormat::R16G16B16A16_Float:
			captureFormat = RhiCaptureFormat{.Encoding = RhiCapturePixelEncoding::Rgba16Float, .BytesPerPixel = 8u};
			return true;
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
			captureFormat = RhiCaptureFormat{.Encoding = RhiCapturePixelEncoding::Rgba8Unorm, .BytesPerPixel = 4u};
			return true;
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			captureFormat = RhiCaptureFormat{.Encoding = RhiCapturePixelEncoding::Bgra8Unorm, .BytesPerPixel = 4u};
			return true;
		default:
			return false;
	}
}
