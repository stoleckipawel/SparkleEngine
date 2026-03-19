#pragma once

#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <cctype>
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
		Meshes
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
		if (sectionName == "Meshes")
		{
			return LevelFileSection::Meshes;
		}

		return LevelFileSection::None;
	}

	inline bool TryParseKeyValueLine(std::string_view line, ParsedLevelLine& parsedLine)
	{
		const std::size_t separatorIndex = line.find('=');
		if (separatorIndex == std::string::npos)
		{
			return false;
		}

		parsedLine.key = Engine::Strings::TrimCopy(line.substr(0, separatorIndex));
		parsedLine.value = Engine::Strings::TrimCopy(line.substr(separatorIndex + 1));
		return true;
	}

	inline bool TryParseBool(std::string_view str, bool& outValue)
	{
		std::string normalized = Engine::Strings::TrimCopy(str);
		std::transform(
		    normalized.begin(),
		    normalized.end(),
		    normalized.begin(),
		    [](unsigned char character)
		    {
			    return static_cast<char>(std::tolower(character));
		    });

		if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on")
		{
			outValue = true;
			return true;
		}

		if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off")
		{
			outValue = false;
			return true;
		}

		return false;
	}
}  // namespace LevelParsing