#pragma once

#include "RHI/Public/Formats/PixelFormat.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

enum class FrameGraphTextureKind : std::uint8_t
{
	Color,
	DepthStencil
};

struct FrameGraphTextureDesc
{
	std::string name;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	PixelFormat format = PixelFormat::Unknown;
	FrameGraphTextureKind kind = FrameGraphTextureKind::Color;
	std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	static FrameGraphTextureDesc CreateDepthTarget(
	    std::string_view name,
	    std::uint32_t width,
	    std::uint32_t height,
	    PixelFormat format) noexcept;

	static FrameGraphTextureDesc CreateColor(std::string_view name, std::uint32_t width, std::uint32_t height, PixelFormat format) noexcept;
};
