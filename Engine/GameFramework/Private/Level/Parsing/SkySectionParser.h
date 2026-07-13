#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/Parsing/LevelParserCommon.h"

#include "Core/Public/Strings/StringUtils.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>

namespace LevelParsing
{
	inline bool ParseSkySectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage)
	{
		if (!levelDesc.sky)
		{
			levelDesc.sky.emplace();
		}
		SceneSkyDesc& sky = *levelDesc.sky;
		if (parsedLine.key == "Enabled")
		{
			if (!Strings::TryParseBool(parsedLine.value, sky.enabled))
			{
				errorMessage = "Invalid sky enabled value";
				return false;
			}
		}
		else if (parsedLine.key == "Color")
		{
			if (!Strings::TryParseFloat3(parsedLine.value, sky.color) || !std::isfinite(sky.color.x) || !std::isfinite(sky.color.y) ||
			    !std::isfinite(sky.color.z) || sky.color.x < 0.0f || sky.color.y < 0.0f || sky.color.z < 0.0f)
			{
				errorMessage = "Invalid sky color";
				return false;
			}
		}
		else if (parsedLine.key == "Intensity")
		{
			if (!Strings::TryParseFloat(parsedLine.value, sky.intensity) || !std::isfinite(sky.intensity) || sky.intensity < 0.0f)
			{
				errorMessage = "Invalid sky intensity";
				return false;
			}
		}
		else if (parsedLine.key == "Texture")
		{
			sky.skyTexture.texturePath = Strings::UnquoteCopy(parsedLine.value);
			sky.skyTexture.textureGroup = TextureGroup::HdrColor;
		}

		return true;
	}

	inline void WriteSkySection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		if (!levelDesc.sky)
		{
			return;
		}

		const SceneSkyDesc& sky = *levelDesc.sky;
		output << std::setprecision(9);
		output << "[Sky]\n";
		output << "Enabled = " << (sky.enabled ? "true" : "false") << "\n";
		output << "Color = " << sky.color.x << ", " << sky.color.y << ", " << sky.color.z << "\n";
		output << "Intensity = " << sky.intensity << "\n";
		output << "Texture = " << sky.skyTexture.texturePath << "\n\n";
	}
}  // namespace LevelParsing
