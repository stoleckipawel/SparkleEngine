#pragma once

#include "FrameGraphResourceHandle.h"

#include <compare>

struct FrameGraphAccelerationStructureHandle
{
	FrameGraphResourceHandle resource = FrameGraphResourceHandle::Invalid();

	constexpr FrameGraphAccelerationStructureHandle() noexcept = default;
	explicit constexpr FrameGraphAccelerationStructureHandle(FrameGraphResourceHandle handle) noexcept : resource(handle) {}

	static constexpr FrameGraphAccelerationStructureHandle Invalid() noexcept { return FrameGraphAccelerationStructureHandle{}; }

	constexpr bool IsValid() const noexcept { return resource.IsValid(); }

	constexpr FrameGraphResourceHandle GetResourceHandle() const noexcept { return resource; }

	constexpr auto operator<=>(const FrameGraphAccelerationStructureHandle&) const noexcept = default;
};
