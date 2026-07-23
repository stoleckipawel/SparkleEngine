#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace LevelParsing
{
	enum class LevelFileSection : std::uint8_t
	{
		None = 0,
		Level,
		Camera,
		Sky,
		Lighting,
		SceneAssets
	};

	struct ParsedLevelLine
	{
		std::string key;
		std::string value;
	};

	LevelFileSection ParseSection(std::string_view line);
}  // namespace LevelParsing
