#pragma once

#include "GameFramework/Public/Level/LevelDesc.h"
#include "Level/LevelParserCommon.h"

#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <string>
#include <string_view>

namespace LevelParsing
{
	inline bool TryParseDirectionalLightFieldKey(std::string_view key, std::size_t& outIndex, std::string_view& outField)
	{
		constexpr std::string_view indexedPrefix = "DirectionalLight";
		if (key.starts_with(indexedPrefix))
		{
			std::size_t cursor = indexedPrefix.size();
			if (cursor >= key.size() || !std::isdigit(static_cast<unsigned char>(key[cursor])))
			{
				return false;
			}

			std::size_t index = 0;
			while (cursor < key.size() && std::isdigit(static_cast<unsigned char>(key[cursor])))
			{
				index = (index * 10) + static_cast<std::size_t>(key[cursor] - '0');
				++cursor;
			}

			if (cursor >= key.size())
			{
				return false;
			}

			outIndex = index;
			outField = key.substr(cursor);
			return true;
		}

		if (key == "Direction" || key == "DirectionalDirection")
		{
			outIndex = 0;
			outField = "Direction";
			return true;
		}

		if (key == "Intensity" || key == "DirectionalIntensity")
		{
			outIndex = 0;
			outField = "Intensity";
			return true;
		}

		if (key == "Color" || key == "DirectionalColor")
		{
			outIndex = 0;
			outField = "Color";
			return true;
		}

		return false;
	}

	inline bool TryParsePointLightFieldKey(std::string_view key, std::size_t& outIndex, std::string_view& outField)
	{
		constexpr std::string_view prefix = "PointLight";
		if (!key.starts_with(prefix))
		{
			return false;
		}

		std::size_t cursor = prefix.size();
		if (cursor >= key.size() || !std::isdigit(static_cast<unsigned char>(key[cursor])))
		{
			return false;
		}

		std::size_t index = 0;
		while (cursor < key.size() && std::isdigit(static_cast<unsigned char>(key[cursor])))
		{
			index = (index * 10) + static_cast<std::size_t>(key[cursor] - '0');
			++cursor;
		}

		if (cursor >= key.size())
		{
			return false;
		}

		outIndex = index;
		outField = key.substr(cursor);
		return true;
	}

	inline bool ParseDirectionalLightField(
	    std::string_view directionalLightField,
	    const ParsedLevelLine& parsedLine,
	    DirectionalLightDesc& directionalLightDesc,
	    std::string& errorMessage)
	{
		if (directionalLightField == "Direction")
		{
			if (!Engine::Strings::TryParseFloat3(parsedLine.value, directionalLightDesc.direction))
			{
				errorMessage = "Invalid directional light direction";
				return false;
			}
			return true;
		}

		if (directionalLightField == "Intensity")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, directionalLightDesc.intensity))
			{
				errorMessage = "Invalid directional light intensity";
				return false;
			}
			return true;
		}

		if (directionalLightField == "Color")
		{
			if (!Engine::Strings::TryParseFloat3(parsedLine.value, directionalLightDesc.color))
			{
				errorMessage = "Invalid directional light color";
				return false;
			}
			return true;
		}

		return false;
	}

	inline bool ParsePointLightField(
	    std::string_view pointLightField,
	    const ParsedLevelLine& parsedLine,
	    PointLightDesc& pointLightDesc,
	    std::string& errorMessage)
	{
		if (pointLightField == "Enabled")
		{
			if (!TryParseBool(parsedLine.value, pointLightDesc.enabled))
			{
				errorMessage = "Invalid point light enabled flag";
				return false;
			}
			return true;
		}

		if (pointLightField == "Position")
		{
			if (!Engine::Strings::TryParseFloat3(parsedLine.value, pointLightDesc.position))
			{
				errorMessage = "Invalid point light position";
				return false;
			}
			return true;
		}

		if (pointLightField == "Intensity")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, pointLightDesc.intensity))
			{
				errorMessage = "Invalid point light intensity";
				return false;
			}
			return true;
		}

		if (pointLightField == "Color")
		{
			if (!Engine::Strings::TryParseFloat3(parsedLine.value, pointLightDesc.color))
			{
				errorMessage = "Invalid point light color";
				return false;
			}
			return true;
		}

		if (pointLightField == "Radius")
		{
			if (!Engine::Strings::TryParseFloat(parsedLine.value, pointLightDesc.radius))
			{
				errorMessage = "Invalid point light radius";
				return false;
			}
			return true;
		}

		return false;
	}

	inline bool ParseLightingSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage)
	{
		std::size_t directionalLightIndex = 0;
		std::string_view directionalLightField;
		if (TryParseDirectionalLightFieldKey(parsedLine.key, directionalLightIndex, directionalLightField))
		{
			if (directionalLightIndex >= LevelLightingDesc::MaxDirectionalLights)
			{
				errorMessage = "Directional light index out of range";
				return false;
			}

			DirectionalLightDesc& directionalLight = levelDesc.lightingDesc.directionalLights[directionalLightIndex];
			if (!ParseDirectionalLightField(directionalLightField, parsedLine, directionalLight, errorMessage))
			{
				return false;
			}

			levelDesc.lightingDesc.directionalLightCount =
			    std::max(levelDesc.lightingDesc.directionalLightCount, static_cast<std::uint32_t>(directionalLightIndex + 1));
			return true;
		}

		std::size_t pointLightIndex = 0;
		std::string_view pointLightField;
		if (!TryParsePointLightFieldKey(parsedLine.key, pointLightIndex, pointLightField))
		{
			return true;
		}

		if (pointLightIndex >= LevelLightingDesc::MaxPointLights)
		{
			errorMessage = "Point light index out of range";
			return false;
		}

		PointLightDesc& pointLight = levelDesc.lightingDesc.pointLights[pointLightIndex];
		if (ParsePointLightField(pointLightField, parsedLine, pointLight, errorMessage))
		{
			levelDesc.lightingDesc.pointLightCount =
			    std::max(levelDesc.lightingDesc.pointLightCount, static_cast<std::uint32_t>(pointLightIndex + 1));
			return true;
		}

		return true;
	}

	inline void WriteLightingSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << std::setprecision(9);
		output << "[Lighting]\n";

		for (std::size_t lightIndex = 0; lightIndex < levelDesc.lightingDesc.directionalLightCount; ++lightIndex)
		{
			const DirectionalLightDesc& directionalLight = levelDesc.lightingDesc.directionalLights[lightIndex];
			output << "DirectionalLight" << lightIndex << "Direction = " << directionalLight.direction.x << ", "
			       << directionalLight.direction.y << ", " << directionalLight.direction.z << "\n";
			output << "DirectionalLight" << lightIndex << "Intensity = " << directionalLight.intensity << "\n";
			output << "DirectionalLight" << lightIndex << "Color = " << directionalLight.color.x << ", " << directionalLight.color.y << ", "
			       << directionalLight.color.z << "\n";
		}

		for (std::size_t lightIndex = 0; lightIndex < levelDesc.lightingDesc.pointLightCount; ++lightIndex)
		{
			const PointLightDesc& pointLight = levelDesc.lightingDesc.pointLights[lightIndex];
			output << "PointLight" << lightIndex << "Enabled = " << (pointLight.enabled ? "true" : "false") << "\n";
			output << "PointLight" << lightIndex << "Position = " << pointLight.position.x << ", " << pointLight.position.y << ", "
			       << pointLight.position.z << "\n";
			output << "PointLight" << lightIndex << "Intensity = " << pointLight.intensity << "\n";
			output << "PointLight" << lightIndex << "Color = " << pointLight.color.x << ", " << pointLight.color.y << ", "
			       << pointLight.color.z << "\n";
			output << "PointLight" << lightIndex << "Radius = " << pointLight.radius << "\n";
		}

		output << "\n";
	}
}  // namespace LevelParsing