#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

constexpr RenderProductHandle ToRenderProductHandle(FrameGraphTextureHandle handle) noexcept
{
	return handle.IsValid() ? RenderProductHandle{static_cast<std::uint64_t>(handle.GetResourceHandle().index) + 1ull}
	                        : RenderProductHandle{};
}

constexpr FrameGraphResourceHandle ToFrameGraphResourceHandle(RenderProductHandle handle) noexcept
{
	return handle ? FrameGraphResourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)} : FrameGraphResourceHandle::Invalid();
}
