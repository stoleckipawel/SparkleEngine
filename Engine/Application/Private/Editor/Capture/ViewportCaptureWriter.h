#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <filesystem>
#include <stop_token>

class ViewportCaptureWriter final
{
public:
	static ViewportCaptureResult Write(
	    ViewportCaptureReadback readback,
	    const std::filesystem::path& outputPath,
	    std::stop_token cancellationToken) noexcept;
};
