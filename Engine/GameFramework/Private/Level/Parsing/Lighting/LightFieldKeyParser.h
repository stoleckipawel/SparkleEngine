#pragma once

#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"

#include <cstddef>
#include <string_view>

namespace LevelParsing
{
	struct ParsedLightFieldKey final
	{
		SceneLightKind Kind = SceneLightKind::Unknown;
		std::size_t Index = 0;
		std::string_view Field;
	};

	ParsedLightFieldKey ParseLightFieldKey(std::string_view key);
}
