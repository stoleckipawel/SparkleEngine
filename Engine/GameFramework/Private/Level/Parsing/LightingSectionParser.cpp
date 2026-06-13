#include "PCH.h"
#include "Level/Parsing/LightingSectionParser.h"

#include "Core/Public/Strings/StringUtils.h"

#include <cctype>
#include <iomanip>
#include <string_view>

namespace
{
	using LevelParsing::ParsedLevelLine;

	struct ParsedLightFieldKey
	{
		SceneLightKind kind = SceneLightKind::Unknown;
		std::size_t index = 0;
		std::string_view field;
	};

	struct LightFieldPrefix
	{
		std::string_view text;
		SceneLightKind kind = SceneLightKind::Unknown;
	};

	bool TryParseIndexedLightFieldKey(std::string_view key, std::string_view indexedPrefix, std::size_t& outIndex, std::string_view& outField)
	{
		if (!key.starts_with(indexedPrefix))
		{
			return false;
		}

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

	bool TryParseLegacyDirectionalLightFieldKey(std::string_view key, ParsedLightFieldKey& outKey)
	{
		if (key == "Direction" || key == "DirectionalDirection")
		{
			outKey.kind = SceneLightKind::Directional;
			outKey.index = 0;
			outKey.field = "Direction";
			return true;
		}

		if (key == "Intensity" || key == "DirectionalIntensity")
		{
			outKey.kind = SceneLightKind::Directional;
			outKey.index = 0;
			outKey.field = "Intensity";
			return true;
		}

		if (key == "Color" || key == "DirectionalColor")
		{
			outKey.kind = SceneLightKind::Directional;
			outKey.index = 0;
			outKey.field = "Color";
			return true;
		}

		if (key == "CastShadow" || key == "DirectionalCastShadow")
		{
			outKey.kind = SceneLightKind::Directional;
			outKey.index = 0;
			outKey.field = "CastShadow";
			return true;
		}

		return false;
	}

	bool TryParseLightFieldKey(std::string_view key, ParsedLightFieldKey& outKey)
	{
		constexpr LightFieldPrefix prefixes[] = {
		    {"DirectionalLight", SceneLightKind::Directional},
		    {"PointLight", SceneLightKind::Point},
		    {"SpotLight", SceneLightKind::Spot},
		};

		for (const LightFieldPrefix& prefix : prefixes)
		{
			std::size_t index = 0;
			std::string_view field;
			if (TryParseIndexedLightFieldKey(key, prefix.text, index, field))
			{
				outKey.kind = prefix.kind;
				outKey.index = index;
				outKey.field = field;
				return true;
			}
		}

		return TryParseLegacyDirectionalLightFieldKey(key, outKey);
	}

	void SetLightPosition(SceneLightCommonDesc& common, const DirectX::XMFLOAT3& position) noexcept
	{
		common.worldTransform._41 = position.x;
		common.worldTransform._42 = position.y;
		common.worldTransform._43 = position.z;
	}

	bool IsSceneLightKind(const SceneLightDesc& light, SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return light.GetDirectional() != nullptr;
			case SceneLightKind::Point:
				return light.GetPoint() != nullptr;
			case SceneLightKind::Spot:
				return light.GetSpot() != nullptr;
			case SceneLightKind::Unknown:
			default:
				return false;
		}
	}

	std::string BuildDefaultLightName(SceneLightKind kind, std::size_t index)
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return "Directional Light " + std::to_string(index + 1);
			case SceneLightKind::Point:
				return "Point Light " + std::to_string(index + 1);
			case SceneLightKind::Spot:
				return "Spot Light " + std::to_string(index + 1);
			case SceneLightKind::Unknown:
			default:
				return {};
		}
	}

	std::string_view GetLightKindName(SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return "directional";
			case SceneLightKind::Point:
				return "point";
			case SceneLightKind::Spot:
				return "spot";
			case SceneLightKind::Unknown:
			default:
				return "unknown";
		}
	}

	SceneLightDesc CreateParsedLight(SceneLightKind kind, std::size_t index)
	{
		SceneLightDesc light;
		light.common.name = BuildDefaultLightName(kind, index);
		switch (kind)
		{
			case SceneLightKind::Directional:
				light.payload = SceneDirectionalLightDesc{};
				break;
			case SceneLightKind::Point:
				light.payload = PointLightDesc{};
				break;
			case SceneLightKind::Spot:
				light.payload = SpotLightDesc{};
				break;
			case SceneLightKind::Unknown:
			default:
				break;
		}
		return light;
	}

	SceneLightDesc& ResolveParsedLight(std::vector<SceneLightDesc>& lights, SceneLightKind kind, std::size_t kindIndex)
	{
		std::size_t currentKindIndex = 0;
		for (SceneLightDesc& light : lights)
		{
			if (!IsSceneLightKind(light, kind))
			{
				continue;
			}

			if (currentKindIndex == kindIndex)
			{
				return light;
			}
			++currentKindIndex;
		}

		while (currentKindIndex <= kindIndex)
		{
			lights.push_back(CreateParsedLight(kind, currentKindIndex));
			if (currentKindIndex == kindIndex)
			{
				return lights.back();
			}
			++currentKindIndex;
		}

		lights.push_back(CreateParsedLight(kind, kindIndex));
		return lights.back();
	}

	bool ParseCommonLightField(std::string_view field, const ParsedLevelLine& parsedLine, SceneLightCommonDesc& common, std::string& errorMessage)
	{
		if (field == "Name")
		{
			common.name = std::string(parsedLine.value);
			return true;
		}

		if (field == "Color")
		{
			if (!Strings::TryParseFloat3(parsedLine.value, common.color))
			{
				errorMessage = "Invalid light color";
				return false;
			}
			return true;
		}

		if (field == "Intensity")
		{
			if (!Strings::TryParseFloat(parsedLine.value, common.intensity))
			{
				errorMessage = "Invalid light intensity";
				return false;
			}
			return true;
		}

		if (field == "Visible")
		{
			bool visible = true;
			if (!Strings::TryParseBool(parsedLine.value, visible))
			{
				errorMessage = "Invalid light Visible value";
				return false;
			}
			common.visible = visible;
			return true;
		}

		if (field == "Position")
		{
			DirectX::XMFLOAT3 position{};
			if (!Strings::TryParseFloat3(parsedLine.value, position))
			{
				errorMessage = "Invalid light position";
				return false;
			}
			SetLightPosition(common, position);
			return true;
		}

		return false;
	}

	bool ParseDirectionalLightField(
	    std::string_view directionalLightField,
	    const ParsedLevelLine& parsedLine,
	    SceneLightDesc& lightDesc,
	    std::string& errorMessage)
	{
		if (lightDesc.common.name.empty())
		{
			lightDesc.common.name = "Directional Light";
		}

		SceneDirectionalLightDesc directional = lightDesc.GetDirectional() != nullptr ? *lightDesc.GetDirectional() : SceneDirectionalLightDesc{};

		if (directionalLightField == "Direction")
		{
			if (!Strings::TryParseFloat3(parsedLine.value, directional.direction))
			{
				errorMessage = "Invalid directional light direction";
				return false;
			}
			lightDesc.payload = directional;
			return true;
		}

		if (ParseCommonLightField(directionalLightField, parsedLine, lightDesc.common, errorMessage))
		{
			lightDesc.payload = directional;
			return true;
		}

		if (!errorMessage.empty())
		{
			return false;
		}

		if (directionalLightField == "CastShadow")
		{
			bool castShadow = true;
			if (!Strings::TryParseBool(parsedLine.value, castShadow))
			{
				errorMessage = "Invalid directional light CastShadow value";
				return false;
			}
			directional.castShadow = castShadow;
			lightDesc.payload = directional;
			return true;
		}

		if (directionalLightField == "AngularDiameterRadians")
		{
			if (!Strings::TryParseFloat(parsedLine.value, directional.angularDiameterRadians))
			{
				errorMessage = "Invalid directional light angular diameter";
				return false;
			}
			lightDesc.payload = directional;
			return true;
		}

		return false;
	}

	bool ParsePointLightField(std::string_view pointLightField, const ParsedLevelLine& parsedLine, SceneLightDesc& lightDesc, std::string& errorMessage)
	{
		if (lightDesc.common.name.empty())
		{
			lightDesc.common.name = "Point Light";
		}

		PointLightDesc point = lightDesc.GetPoint() != nullptr ? *lightDesc.GetPoint() : PointLightDesc{};

		if (ParseCommonLightField(pointLightField, parsedLine, lightDesc.common, errorMessage))
		{
			lightDesc.payload = point;
			return true;
		}

		if (!errorMessage.empty())
		{
			return false;
		}

		if (pointLightField == "Range")
		{
			if (!Strings::TryParseFloat(parsedLine.value, point.range))
			{
				errorMessage = "Invalid point light range";
				return false;
			}
			lightDesc.payload = point;
			return true;
		}

		if (pointLightField == "SourceRadius")
		{
			if (!Strings::TryParseFloat(parsedLine.value, point.sourceRadius))
			{
				errorMessage = "Invalid point light source radius";
				return false;
			}
			lightDesc.payload = point;
			return true;
		}

		if (pointLightField == "CastShadow")
		{
			bool castShadow = true;
			if (!Strings::TryParseBool(parsedLine.value, castShadow))
			{
				errorMessage = "Invalid point light CastShadow value";
				return false;
			}
			point.castShadow = castShadow;
			lightDesc.payload = point;
			return true;
		}

		return false;
	}

	bool ParseSpotLightField(std::string_view spotLightField, const ParsedLevelLine& parsedLine, SceneLightDesc& lightDesc, std::string& errorMessage)
	{
		if (lightDesc.common.name.empty())
		{
			lightDesc.common.name = "Spot Light";
		}

		SpotLightDesc spot = lightDesc.GetSpot() != nullptr ? *lightDesc.GetSpot() : SpotLightDesc{};

		if (ParseCommonLightField(spotLightField, parsedLine, lightDesc.common, errorMessage))
		{
			lightDesc.payload = spot;
			return true;
		}

		if (!errorMessage.empty())
		{
			return false;
		}

		if (spotLightField == "Direction")
		{
			if (!Strings::TryParseFloat3(parsedLine.value, spot.direction))
			{
				errorMessage = "Invalid spot light direction";
				return false;
			}
			lightDesc.payload = spot;
			return true;
		}

		if (spotLightField == "Range")
		{
			if (!Strings::TryParseFloat(parsedLine.value, spot.range))
			{
				errorMessage = "Invalid spot light range";
				return false;
			}
			lightDesc.payload = spot;
			return true;
		}

		if (spotLightField == "SourceRadius")
		{
			if (!Strings::TryParseFloat(parsedLine.value, spot.sourceRadius))
			{
				errorMessage = "Invalid spot light source radius";
				return false;
			}
			lightDesc.payload = spot;
			return true;
		}

		if (spotLightField == "InnerConeAngleRadians")
		{
			if (!Strings::TryParseFloat(parsedLine.value, spot.innerConeAngleRadians))
			{
				errorMessage = "Invalid spot light inner cone angle";
				return false;
			}
			lightDesc.payload = spot;
			return true;
		}

		if (spotLightField == "OuterConeAngleRadians")
		{
			if (!Strings::TryParseFloat(parsedLine.value, spot.outerConeAngleRadians))
			{
				errorMessage = "Invalid spot light outer cone angle";
				return false;
			}
			lightDesc.payload = spot;
			return true;
		}

		if (spotLightField == "CastShadow")
		{
			bool castShadow = true;
			if (!Strings::TryParseBool(parsedLine.value, castShadow))
			{
				errorMessage = "Invalid spot light CastShadow value";
				return false;
			}
			spot.castShadow = castShadow;
			lightDesc.payload = spot;
			return true;
		}

		return false;
	}

	void WriteCommonLightFields(std::ofstream& output, std::string_view prefix, const SceneLightCommonDesc& common)
	{
		if (!common.name.empty())
		{
			output << prefix << "Name = " << common.name << "\n";
		}
		output << prefix << "Position = " << common.worldTransform._41 << ", " << common.worldTransform._42 << ", " << common.worldTransform._43
		       << "\n";
		output << prefix << "Intensity = " << common.intensity << "\n";
		output << prefix << "Color = " << common.color.x << ", " << common.color.y << ", " << common.color.z << "\n";
		output << prefix << "Visible = " << (common.visible ? "true" : "false") << "\n";
	}
}

namespace LevelParsing
{
	bool ParseLightingSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc, std::string& errorMessage)
	{
		ParsedLightFieldKey lightKey;
		if (!TryParseLightFieldKey(parsedLine.key, lightKey))
		{
			return true;
		}

		SceneLightDesc& light = ResolveParsedLight(levelDesc.lights, lightKey.kind, lightKey.index);
		bool parsed = false;
		switch (lightKey.kind)
		{
			case SceneLightKind::Directional:
				parsed = ParseDirectionalLightField(lightKey.field, parsedLine, light, errorMessage);
				break;
			case SceneLightKind::Point:
				parsed = ParsePointLightField(lightKey.field, parsedLine, light, errorMessage);
				break;
			case SceneLightKind::Spot:
				parsed = ParseSpotLightField(lightKey.field, parsedLine, light, errorMessage);
				break;
			case SceneLightKind::Unknown:
			default:
				return true;
		}

		if (!parsed && errorMessage.empty())
		{
			errorMessage = "Unsupported " + std::string(GetLightKindName(lightKey.kind)) + " light field: " + std::string(lightKey.field);
		}

		return parsed;
	}

	void WriteLightingSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << std::setprecision(9);
		output << "[Lighting]\n";

		std::size_t directionalIndex = 0;
		std::size_t pointIndex = 0;
		std::size_t spotIndex = 0;
		for (const SceneLightDesc& light : levelDesc.lights)
		{
			if (const SceneDirectionalLightDesc* directional = light.GetDirectional())
			{
				const std::string prefix = "DirectionalLight" + std::to_string(directionalIndex++);
				WriteCommonLightFields(output, prefix, light.common);
				output << prefix << "Direction = " << directional->direction.x << ", " << directional->direction.y << ", "
				       << directional->direction.z << "\n";
				output << prefix << "AngularDiameterRadians = " << directional->angularDiameterRadians << "\n";
				output << prefix << "CastShadow = " << (directional->castShadow ? "true" : "false") << "\n";
				continue;
			}

			if (const PointLightDesc* point = light.GetPoint())
			{
				const std::string prefix = "PointLight" + std::to_string(pointIndex++);
				WriteCommonLightFields(output, prefix, light.common);
				output << prefix << "Range = " << point->range << "\n";
				output << prefix << "SourceRadius = " << point->sourceRadius << "\n";
				output << prefix << "CastShadow = " << (point->castShadow ? "true" : "false") << "\n";
				continue;
			}

			if (const SpotLightDesc* spot = light.GetSpot())
			{
				const std::string prefix = "SpotLight" + std::to_string(spotIndex++);
				WriteCommonLightFields(output, prefix, light.common);
				output << prefix << "Direction = " << spot->direction.x << ", " << spot->direction.y << ", " << spot->direction.z << "\n";
				output << prefix << "Range = " << spot->range << "\n";
				output << prefix << "SourceRadius = " << spot->sourceRadius << "\n";
				output << prefix << "InnerConeAngleRadians = " << spot->innerConeAngleRadians << "\n";
				output << prefix << "OuterConeAngleRadians = " << spot->outerConeAngleRadians << "\n";
				output << prefix << "CastShadow = " << (spot->castShadow ? "true" : "false") << "\n";
			}
		}

		output << "\n";
	}
}  // namespace LevelParsing
