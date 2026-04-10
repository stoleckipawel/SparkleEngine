#pragma once

#include "Core/Public/Strings/StringUtils.h"

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
		Lighting,
		SceneAssets
	};

	struct ParsedLevelLine
	{
		std::string key;
		std::string value;
	};

	inline LevelFileSection ParseSection(std::string_view line)
	{
		const std::string sectionName = Engine::Strings::TrimCopy(line.substr(1, line.size() - 2));
		if (sectionName == "Level")
		{
			return LevelFileSection::Level;
		}
		if (sectionName == "Camera")
		{
			return LevelFileSection::Camera;
		}
		if (sectionName == "Lighting")
		{
			return LevelFileSection::Lighting;
		}
		if (sectionName == "SceneAssets")
		{
			return LevelFileSection::SceneAssets;
		}

		return LevelFileSection::None;
	}

}  // namespace LevelParsing