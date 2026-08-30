#pragma once

#include <DirectXMath.h>

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
	ParsedLevelLine ParseField(std::string_view line);
	float ParseFloat(std::string_view value, std::string_view fieldName);
	DirectX::XMFLOAT3 ParseFloat3(std::string_view value, std::string_view fieldName);
	bool ParseBool(std::string_view value, std::string_view fieldName);
}
