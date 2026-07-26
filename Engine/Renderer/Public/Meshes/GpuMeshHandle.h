#pragma once

#include "../RendererAPI.h"

#include <compare>
#include <cstdint>

struct SPARKLE_RENDERER_API GpuMeshHandle final
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
	constexpr auto operator<=>(const GpuMeshHandle&) const noexcept = default;
};
