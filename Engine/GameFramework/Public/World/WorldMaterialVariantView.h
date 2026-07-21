#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialVariant.h"

#include <string>
#include <vector>

struct SPARKLE_ENGINE_API WorldMaterialVariantView final
{
	std::vector<std::string> Names;
	MaterialVariantIndex Active = kInvalidMaterialVariantIndex;
};
