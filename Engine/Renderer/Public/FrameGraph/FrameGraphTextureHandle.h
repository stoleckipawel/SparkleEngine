#pragma once

#include "FrameGraphResourceHandle.h"

#include <compare>

struct FrameGraphTextureHandle
{
	FrameGraphResourceHandle resource = FrameGraphResourceHandle::Invalid();

	constexpr FrameGraphTextureHandle() noexcept = default;
	explicit constexpr FrameGraphTextureHandle(FrameGraphResourceHandle handle) noexcept : resource(handle) {}

	static constexpr FrameGraphTextureHandle Invalid() noexcept { return FrameGraphTextureHandle{}; }

	constexpr bool IsValid() const noexcept { return resource.IsValid(); }

	constexpr FrameGraphResourceHandle GetResourceHandle() const noexcept { return resource; }

	constexpr auto operator<=>(const FrameGraphTextureHandle&) const noexcept = default;
};