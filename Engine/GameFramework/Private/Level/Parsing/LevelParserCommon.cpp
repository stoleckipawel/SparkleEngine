#include "PCH.h"

#include "Level/Parsing/LevelParserCommon.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <format>

namespace LevelParsing
{
	LevelFileSection ParseSection(std::string_view line)
	{
		const std::string sectionName = Strings::TrimCopy(line.substr(1, line.size() - 2));
		if (sectionName == "Level") return LevelFileSection::Level;
		if (sectionName == "Camera") return LevelFileSection::Camera;
		if (sectionName == "Sky") return LevelFileSection::Sky;
		if (sectionName == "Lighting") return LevelFileSection::Lighting;
		if (sectionName == "SceneAssets") return LevelFileSection::SceneAssets;
		throw Diagnostics::Error(std::format("Unsupported level section '{}'.", sectionName));
	}

	ParsedLevelLine ParseField(std::string_view line)
	{
		std::string_view key;
		std::string_view value;
		if (!Strings::TrySplitKeyValue(line, '=', key, value))
			throw Diagnostics::Error("Malformed level field.");
		return ParsedLevelLine{.key = std::string(key), .value = std::string(value)};
	}

	float ParseFloat(std::string_view value, std::string_view fieldName)
	{
		float parsed = 0.0f;
		if (!Strings::TryParseFloat(value, parsed))
			throw Diagnostics::Error(std::format("Invalid {}.", fieldName));
		return parsed;
	}

	DirectX::XMFLOAT3 ParseFloat3(std::string_view value, std::string_view fieldName)
	{
		DirectX::XMFLOAT3 parsed;
		if (!Strings::TryParseFloat3(value, parsed))
			throw Diagnostics::Error(std::format("Invalid {}.", fieldName));
		return parsed;
	}

	bool ParseBool(std::string_view value, std::string_view fieldName)
	{
		bool parsed = false;
		if (!Strings::TryParseBool(value, parsed))
			throw Diagnostics::Error(std::format("Invalid {}.", fieldName));
		return parsed;
	}
}  // namespace LevelParsing
