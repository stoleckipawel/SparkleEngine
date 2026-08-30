#include "PCH.h"

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include "RHI/Public/Capture/RhiCaptureService.h"

bool WriteViewportCaptureBmp(const ViewportCaptureReadback& readback) noexcept
{
	RhiCaptureReadback rhiReadback;
	rhiReadback.Pixels = readback.Pixels;
	rhiReadback.Width = readback.Width;
	rhiReadback.Height = readback.Height;
	rhiReadback.RowPitch = readback.RowPitch;
	rhiReadback.Format = readback.Format;
	return WriteRhiCaptureBmp(rhiReadback, readback.Result.ArtifactPath);
}
