#pragma once

#include <cstddef>
#include <filesystem>

#include "RHI/Public/Formats/PixelFormat.h"

bool WriteRhiBmp(
    const std::filesystem::path& outputPath,
    const std::byte* sourcePixels,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t sourceRowPitch,
    PixelFormat sourceFormat) noexcept;
