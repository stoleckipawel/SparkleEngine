#pragma once

#include "Renderer/Public/FrameGraph/ResourceHandle.h"

#include <compare>

struct BufferHandle
{
	ResourceHandle resource = ResourceHandle::Invalid();

	constexpr BufferHandle() noexcept = default;
	explicit constexpr BufferHandle(ResourceHandle handle) noexcept : resource(handle) {}

	static constexpr BufferHandle Invalid() noexcept { return BufferHandle{}; }

	constexpr bool IsValid() const noexcept { return resource.IsValid(); }

	constexpr ResourceHandle GetResourceHandle() const noexcept { return resource; }

	constexpr auto operator<=>(const BufferHandle&) const noexcept = default;
};