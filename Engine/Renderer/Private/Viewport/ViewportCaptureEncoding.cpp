#include "PCH.h"

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include "RHI/Public/Capture/RhiCaptureService.h"

class ViewportCaptureEncoding final
{
  public:
	static RhiBmpSourceFormat ToRhiFormat(
	    ViewportCapturePixelFormat format) noexcept
	{
		switch (format)
		{
			case ViewportCapturePixelFormat::Rgba32Float:
				return RhiBmpSourceFormat::Rgba32Float;
			case ViewportCapturePixelFormat::Rgba16Float:
				return RhiBmpSourceFormat::Rgba16Float;
			case ViewportCapturePixelFormat::Bgra8Unorm:
				return RhiBmpSourceFormat::Bgra8Unorm;
			case ViewportCapturePixelFormat::Rgba8Unorm:
			default:
				return RhiBmpSourceFormat::Rgba8Unorm;
		}
	}
};

bool WriteViewportCaptureBmp(
    const ViewportCaptureReadback& readback) noexcept
{
	RhiCaptureReadback rhiReadback;
	rhiReadback.Pixels = readback.Pixels;
	rhiReadback.Width = readback.Width;
	rhiReadback.Height = readback.Height;
	rhiReadback.RowPitch = readback.RowPitch;
	rhiReadback.Format =
	    ViewportCaptureEncoding::ToRhiFormat(readback.Format);
	return WriteRhiCaptureBmp(
	    rhiReadback,
	    readback.Result.ArtifactPath);
}
