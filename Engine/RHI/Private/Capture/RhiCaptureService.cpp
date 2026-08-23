#include "Capture/RhiCaptureService.h"

#include "Capture/RhiCaptureFormat.h"
#include "Capture/RhiBmpWriter.h"

RhiCaptureService::~RhiCaptureService() noexcept = default;

RhiCaptureTicket::operator bool() const noexcept
{
	return Value != 0;
}

bool IsRhiCapturePixelFormat(PixelFormat format) noexcept
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

bool WriteRhiCaptureBmp(const RhiCaptureReadback& readback, const std::filesystem::path& outputPath) noexcept
{
	return !readback.Pixels.empty()
	    && WriteRhiBmp(outputPath, readback.Pixels.data(), readback.Width, readback.Height, readback.RowPitch, readback.Format);
}
