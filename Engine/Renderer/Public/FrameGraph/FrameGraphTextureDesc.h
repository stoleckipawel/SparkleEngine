#pragma once

#include "RHIConfig.h"

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
	DXGI_FORMAT format = RHISettings::DepthStencilFormat;
	FrameGraphTextureKind kind = FrameGraphTextureKind::Color;

	static FrameGraphTextureDesc CreateDepthStencil(std::string_view name, std::uint32_t width, std::uint32_t height) noexcept
	{
		return FrameGraphTextureDesc{
		    std::string(name),
		    width,
		    height,
		    RHISettings::DepthStencilFormat,
		    FrameGraphTextureKind::DepthStencil};
	}

	static FrameGraphTextureDesc CreateColor(std::string_view name, std::uint32_t width, std::uint32_t height, DXGI_FORMAT format) noexcept
	{
		return FrameGraphTextureDesc{std::string(name), width, height, format, FrameGraphTextureKind::Color};
	}
};