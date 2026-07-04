#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

enum class RhiBmpSourceFormat : std::uint8_t
{
	Rgba32Float = 0,
	Rgba8Unorm,
	Bgra8Unorm
};

bool WriteRhiBmp(
    const std::filesystem::path& outputPath,
    const std::byte* sourcePixels,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t sourceRowPitch,
    RhiBmpSourceFormat sourceFormat) noexcept;
