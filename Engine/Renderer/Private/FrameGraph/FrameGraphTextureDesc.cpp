#include "PCH.h"

#include "FrameGraph/FrameGraphTextureDesc.h"

FrameGraphTextureDesc FrameGraphTextureDesc::CreateDepthTarget(
    std::string_view name,
    std::uint32_t width,
    std::uint32_t height,
    PixelFormat format) noexcept
{
	return FrameGraphTextureDesc{std::string(name), width, height, format, FrameGraphTextureKind::DepthStencil, 1, {0.0f, 0.0f, 0.0f, 1.0f}};
}

FrameGraphTextureDesc FrameGraphTextureDesc::CreateColor(
    std::string_view name,
    std::uint32_t width,
    std::uint32_t height,
    PixelFormat format) noexcept
{
	return FrameGraphTextureDesc{std::string(name), width, height, format, FrameGraphTextureKind::Color, 1, {0.0f, 0.0f, 0.0f, 1.0f}};
}
