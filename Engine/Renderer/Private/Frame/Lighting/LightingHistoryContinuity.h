#pragma once

#include <cstdint>

struct FrameContext;

struct LightingHistoryContinuity final
{
	std::uint64_t ReferenceLighting = 0u;
	std::uint64_t RestirLighting = 0u;

	bool operator==(const LightingHistoryContinuity&) const = default;
};

LightingHistoryContinuity BuildLightingHistoryContinuity(const FrameContext& frame) noexcept;
