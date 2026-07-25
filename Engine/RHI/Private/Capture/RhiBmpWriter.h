#pragma once

#include <cstddef>
#include <filesystem>

#include "Capture/RhiCaptureService.h"

bool WriteRhiBmp(
    const std::filesystem::path& outputPath,
    const std::byte* sourcePixels,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t sourceRowPitch,
    RhiBmpSourceFormat sourceFormat) noexcept;
