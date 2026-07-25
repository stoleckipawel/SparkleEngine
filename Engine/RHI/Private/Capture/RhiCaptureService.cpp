#include "Capture/RhiCaptureService.h"

#include "Capture/RhiBmpWriter.h"

RhiCaptureService::~RhiCaptureService() noexcept = default;

RhiCaptureTicket::operator bool() const noexcept
{
	return Value != 0;
}

bool WriteRhiCaptureBmp(
    const RhiCaptureReadback& readback,
    const std::filesystem::path& outputPath) noexcept
{
	return !readback.Pixels.empty() &&
	       WriteRhiBmp(
	           outputPath,
	           readback.Pixels.data(),
	           readback.Width,
	           readback.Height,
	           readback.RowPitch,
	           readback.Format);
}
