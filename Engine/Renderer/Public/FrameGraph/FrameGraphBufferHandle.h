#pragma once

#include "FrameGraphResourceHandle.h"

#include <compare>

struct FrameGraphBufferHandle
{
	FrameGraphResourceHandle resource = FrameGraphResourceHandle::Invalid();

	constexpr FrameGraphBufferHandle() noexcept = default;
	explicit constexpr FrameGraphBufferHandle(FrameGraphResourceHandle handle) noexcept : resource(handle) {}

	static constexpr FrameGraphBufferHandle Invalid() noexcept { return FrameGraphBufferHandle{}; }

	constexpr bool IsValid() const noexcept { return resource.IsValid(); }

	constexpr FrameGraphResourceHandle GetResourceHandle() const noexcept { return resource; }

	constexpr auto operator<=>(const FrameGraphBufferHandle&) const noexcept = default;
};