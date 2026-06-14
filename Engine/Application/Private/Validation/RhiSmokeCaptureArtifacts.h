#pragma once

#include "Renderer/Public/Diagnostics/RendererSmokeDiagnostics.h"
#include "RHI/Public/Capture/RhiCaptureService.h"

#include <filesystem>

struct RhiSmokeCaptureArtifactRequest final
{
	RhiCaptureResult CaptureResult;
	RendererSmokeDiagnosticsSnapshot Diagnostics;
	std::filesystem::path CapturePath;
	std::filesystem::path MetadataPath;
	std::filesystem::path TimingCsvPath;
};

namespace RhiSmokeCaptureArtifacts
{
	void Write(const RhiSmokeCaptureArtifactRequest& request) noexcept;
}
