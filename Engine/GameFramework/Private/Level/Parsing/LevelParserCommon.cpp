#include "PCH.h"

#include "Level/Parsing/LevelParserCommon.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <array>
#include <format>
#include <sstream>

namespace LevelParsing
{
	static bool TryParseFloatValue(std::string_view value, float& outValue)
	{
		const std::string trimmed = Strings::TrimCopy(value);
		if (trimmed.empty())
		{
			return false;
		}

		try
		{
			std::size_t parsedLength = 0;
			const float parsedValue = std::stof(trimmed, &parsedLength);
			if (parsedLength != trimmed.size())
			{
				return false;
			}
			outValue = parsedValue;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	static bool TryParseFloat3Value(std::string_view value, DirectX::XMFLOAT3& outValue)
	{
		constexpr std::size_t componentCount = 3;
		std::stringstream stream{std::string(value)};
		std::string segment;
		std::array<float, componentCount> values{};
		for (float& component : values)
		{
			if (!std::getline(stream, segment, ',') || !TryParseFloatValue(segment, component))
			{
				return false;
			}
		}
		if (std::getline(stream, segment, ','))
		{
			return false;
		}

		outValue = {values.front(), values[1], values.back()};
		return true;
	}

	LevelFileSection ParseSection(std::string_view line)
	{
		const std::string sectionName = Strings::TrimCopy(line.substr(1, line.size() - 2));
		if (sectionName == "Level")
			return LevelFileSection::Level;
		if (sectionName == "Camera")
			return LevelFileSection::Camera;
		if (sectionName == "Sky")
			return LevelFileSection::Sky;
		if (sectionName == "Lighting")
			return LevelFileSection::Lighting;
		if (sectionName == "SceneAssets")
			return LevelFileSection::SceneAssets;
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
		if (!TryParseFloatValue(value, parsed))
			throw Diagnostics::Error(std::format("Invalid {}.", fieldName));
		return parsed;
	}

	DirectX::XMFLOAT3 ParseFloat3(std::string_view value, std::string_view fieldName)
	{
		DirectX::XMFLOAT3 parsed;
		if (!TryParseFloat3Value(value, parsed))
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
}
