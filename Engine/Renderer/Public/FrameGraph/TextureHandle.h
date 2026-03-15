#pragma once

#include "Renderer/Public/FrameGraph/ResourceHandle.h"

#include <compare>

struct TextureHandle
{
	ResourceHandle resource = ResourceHandle::Invalid();

	constexpr TextureHandle() noexcept = default;
	explicit constexpr TextureHandle(ResourceHandle handle) noexcept : resource(handle) {}

	static constexpr TextureHandle Invalid() noexcept { return TextureHandle{}; }

	constexpr bool IsValid() const noexcept { return resource.IsValid(); }

	constexpr ResourceHandle GetResourceHandle() const noexcept { return resource; }

	constexpr auto operator<=>(const TextureHandle&) const noexcept = default;
};