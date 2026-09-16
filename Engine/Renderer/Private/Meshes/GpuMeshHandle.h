#pragma once

#include <compare>
#include <cstdint>

struct GpuMeshHandle final
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
	constexpr auto operator<=>(const GpuMeshHandle&) const noexcept = default;
};
