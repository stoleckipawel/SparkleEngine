#include "PCH.h"

#include "Capture/RhiCaptureFormat.h"

bool IsRhiCaptureFormatSupported(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R32G32B32A32_Float:
		case PixelFormat::R16G16B16A16_Float:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return true;
		default:
			return false;
	}
}
