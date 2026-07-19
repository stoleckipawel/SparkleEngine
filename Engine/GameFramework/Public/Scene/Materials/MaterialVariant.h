#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <limits>
#include <string>

using MaterialVariantIndex = std::uint32_t;
inline constexpr MaterialVariantIndex kInvalidMaterialVariantIndex = (std::numeric_limits<MaterialVariantIndex>::max)();

struct SPARKLE_ENGINE_API MaterialVariantDesc final
{
	std::string name;
	std::uint32_t sourceVariantIndex = 0;
};
