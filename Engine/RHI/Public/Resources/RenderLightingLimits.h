#pragma once

#include <cstddef>

namespace RenderLightingLimits
{
	inline constexpr std::size_t MaxDirectionalLights = 2;
	inline constexpr std::size_t MaxPointLights = 512;
	inline constexpr std::size_t MaxSpotLights = 512;
}

