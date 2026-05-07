#pragma once

#include "Config/RenderConfig.h"
#include "Config/DepthConvention.h"

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
	PixelFormat format = RenderConfig::DepthStencilFormat;
	FrameGraphTextureKind kind = FrameGraphTextureKind::Color;
	std::array<float, 4> clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	static FrameGraphTextureDesc CreateDepthStencil(std::string_view name, std::uint32_t width, std::uint32_t height) noexcept
	{
		return FrameGraphTextureDesc{
		    std::string(name),
		    width,
		    height,
		    RenderConfig::DepthStencilFormat,
		    FrameGraphTextureKind::DepthStencil,
		    {0.0f, 0.0f, 0.0f, 1.0f}};
	}

	static FrameGraphTextureDesc CreateColor(std::string_view name, std::uint32_t width, std::uint32_t height, PixelFormat format) noexcept
	{
		return FrameGraphTextureDesc{std::string(name), width, height, format, FrameGraphTextureKind::Color, {0.0f, 0.0f, 0.0f, 1.0f}};
	}

	static FrameGraphTextureDesc CreateDepth(std::string_view name, std::uint32_t width, std::uint32_t height, PixelFormat format) noexcept
	{
		const float clearDepth = DepthConvention::GetClearDepth();
		return FrameGraphTextureDesc{
		    std::string(name),
		    width,
		    height,
		    format,
		    FrameGraphTextureKind::Color,
		    {clearDepth, clearDepth, clearDepth, 1.0f}};
	}
};