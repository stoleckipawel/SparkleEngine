#include "PCH.h"

#include "Editor/Capture/ViewportCaptureWriter.h"

#include "Core/Public/Files/FileUtils.h"
#include "Editor/Capture/ImageEncoding.h"

ViewportCaptureResult ViewportCaptureWriter::Write(
    ViewportCaptureReadback readback,
    const std::filesystem::path& outputPath,
    std::stop_token cancellationToken) noexcept
{
	ViewportCaptureResult result = readback.Result;
	if (cancellationToken.stop_requested())
	{
		result.Status = ViewportCaptureStatus::Failed;
		result.FailureReason = "Viewport capture write was cancelled.";
		return result;
	}

	const ImageBufferView image{
	    .Pixels = readback.Pixels,
	    .Width = readback.Width,
	    .Height = readback.Height,
	    .RowPitch = readback.RowPitch,
	    .Format = readback.Format};
	std::vector<std::byte> encodedBytes;
	std::string failureReason;
	const bool encoded = ImageEncoding::EncodeBmp(image, encodedBytes, failureReason);
	const bool written =
	    encoded && !cancellationToken.stop_requested() && Files::TryWriteAllBytesAtomic(outputPath, encodedBytes, failureReason);
	result.Status = written ? ViewportCaptureStatus::Succeeded : ViewportCaptureStatus::Failed;
	if (!written)
	{
		result.FailureReason = cancellationToken.stop_requested()
		    ? "Viewport capture write was cancelled."
		    : (failureReason.empty() ? "Viewport capture BMP encoding failed" : std::move(failureReason));
	}
	return result;
}
