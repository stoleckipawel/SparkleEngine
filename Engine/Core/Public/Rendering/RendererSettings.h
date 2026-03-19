#pragma once

#include <cstddef>

namespace RendererSettings::Lights
{
	inline constexpr std::size_t MaxDirectionalLights = 2;
	inline constexpr std::size_t MaxLocalLights = 256;
	inline constexpr std::size_t MaxPointLights = MaxLocalLights;
}  // namespace RendererSettings::Lights