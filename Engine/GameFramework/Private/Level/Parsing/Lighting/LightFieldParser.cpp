#include "PCH.h"

#include "LightFieldParser.h"

#include "Core/Public/Strings/StringUtils.h"

namespace LevelParsing
{

		void SetLightPosition(SceneLightCommonDesc& common, const DirectX::XMFLOAT3& position) noexcept
		{
			common.worldTransform._41 = position.x;
			common.worldTransform._42 = position.y;
			common.worldTransform._43 = position.z;
		}

		bool IsLightKind(const SceneLightDesc& light, SceneLightKind kind) noexcept
		{
			switch (kind)
			{
				case SceneLightKind::Directional: return light.GetDirectional() != nullptr;
				case SceneLightKind::Point: return light.GetPoint() != nullptr;
				case SceneLightKind::Spot: return light.GetSpot() != nullptr;
				case SceneLightKind::Rect: return light.GetRect() != nullptr;
				case SceneLightKind::Unknown:
				default: return false;
			}
		}

		std::string BuildDefaultLightName(SceneLightKind kind, std::size_t index)
		{
			switch (kind)
			{
				case SceneLightKind::Directional: return "Directional Light " + std::to_string(index + 1);
				case SceneLightKind::Point: return "Point Light " + std::to_string(index + 1);
				case SceneLightKind::Spot: return "Spot Light " + std::to_string(index + 1);
				case SceneLightKind::Rect: return "Rect Light " + std::to_string(index + 1);
				case SceneLightKind::Unknown:
				default: return {};
			}
		}

		SceneLightDesc CreateLight(SceneLightKind kind, std::size_t index)
		{
			SceneLightDesc light;
			light.common.name = BuildDefaultLightName(kind, index);
			switch (kind)
			{
				case SceneLightKind::Directional: light.payload = SceneDirectionalLightDesc{}; break;
				case SceneLightKind::Point: light.payload = PointLightDesc{}; break;
				case SceneLightKind::Spot: light.payload = SpotLightDesc{}; break;
				case SceneLightKind::Rect: light.payload = RectLightDesc{}; break;
				case SceneLightKind::Unknown:
				default: break;
			}
			return light;
		}

		SceneLightDesc& ResolveLight(std::vector<SceneLightDesc>& lights, SceneLightKind kind, std::size_t kindIndex)
		{
			std::size_t currentKindIndex = 0;
			for (SceneLightDesc& light : lights)
			{
				if (!IsLightKind(light, kind)) continue;
				if (currentKindIndex == kindIndex) return light;
				++currentKindIndex;
			}

			while (currentKindIndex <= kindIndex)
			{
				lights.push_back(CreateLight(kind, currentKindIndex));
				if (currentKindIndex == kindIndex) return lights.back();
				++currentKindIndex;
			}
			lights.push_back(CreateLight(kind, kindIndex));
			return lights.back();
		}

		bool ParseCommonField(
		    std::string_view field,
		    const ParsedLevelLine& parsedLine,
		    SceneLightCommonDesc& common,
		    std::string& errorMessage,
		    std::string_view intensityField)
		{
			if (field == "Name")
			{
				common.name = std::string(parsedLine.value);
				return true;
			}
			if (field == "Color")
			{
				if (Strings::TryParseFloat3(parsedLine.value, common.color)) return true;
				errorMessage = "Invalid light color";
				return false;
			}
			if (field == intensityField)
			{
				if (Strings::TryParseFloat(parsedLine.value, common.intensity)) return true;
				errorMessage = "Invalid light intensity";
				return false;
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

		bool ParseDirectionalField(
		    std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light, std::string& error)
		{
			SceneDirectionalLightDesc value = light.GetDirectional() ? *light.GetDirectional() : SceneDirectionalLightDesc{};
			if (field == "Direction")
			{
				if (!Strings::TryParseFloat3(line.value, value.direction))
				{
					error = "Invalid directional light direction";
					return false;
				}
			}
			else if (ParseCommonField(field, line, light.common, error, "IntensityLux")) {}
			else if (!error.empty()) return false;
			else if (field == "CastShadow")
			{
				if (!Strings::TryParseBool(line.value, value.castShadow))
				{
					error = "Invalid directional light CastShadow value";
					return false;
				}
			}
			else if (field == "AngularDiameterRadians")
			{
				if (!Strings::TryParseFloat(line.value, value.angularDiameterRadians))
				{
					error = "Invalid directional light angular diameter";
					return false;
				}
			}
			else return false;
			light.payload = value;
			return true;
		}

		bool ParsePointField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light, std::string& error)
		{
			PointLightDesc value = light.GetPoint() ? *light.GetPoint() : PointLightDesc{};
			if (ParseCommonField(field, line, light.common, error, "IntensityCandela")) {}
			else if (!error.empty()) return false;
			else if (field == "Range")
			{
				if (!Strings::TryParseFloat(line.value, value.range)) { error = "Invalid point light range"; return false; }
			}
			else if (field == "SourceRadius")
			{
				if (!Strings::TryParseFloat(line.value, value.sourceRadius)) { error = "Invalid point light source radius"; return false; }
			}
			else if (field == "CastShadow")
			{
				if (!Strings::TryParseBool(line.value, value.castShadow)) { error = "Invalid point light CastShadow value"; return false; }
			}
			else return false;
			light.payload = value;
			return true;
		}

		bool ParseSpotField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light, std::string& error)
		{
			SpotLightDesc value = light.GetSpot() ? *light.GetSpot() : SpotLightDesc{};
			if (ParseCommonField(field, line, light.common, error, "IntensityCandela")) {}
			else if (!error.empty()) return false;
			else if (field == "Direction")
			{
				if (!Strings::TryParseFloat3(line.value, value.direction)) { error = "Invalid spot light direction"; return false; }
			}
			else if (field == "Range")
			{
				if (!Strings::TryParseFloat(line.value, value.range)) { error = "Invalid spot light range"; return false; }
			}
			else if (field == "SourceRadius")
			{
				if (!Strings::TryParseFloat(line.value, value.sourceRadius)) { error = "Invalid spot light source radius"; return false; }
			}
			else if (field == "InnerConeAngleRadians")
			{
				if (!Strings::TryParseFloat(line.value, value.innerConeAngleRadians)) { error = "Invalid spot light inner cone angle"; return false; }
			}
			else if (field == "OuterConeAngleRadians")
			{
				if (!Strings::TryParseFloat(line.value, value.outerConeAngleRadians)) { error = "Invalid spot light outer cone angle"; return false; }
			}
			else if (field == "CastShadow")
			{
				if (!Strings::TryParseBool(line.value, value.castShadow)) { error = "Invalid spot light CastShadow value"; return false; }
			}
			else return false;
			light.payload = value;
			return true;
		}

		bool ParseRectField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light, std::string& error)
		{
			RectLightDesc value = light.GetRect() ? *light.GetRect() : RectLightDesc{};
			if (field == "LuminanceCdPerM2")
			{
				if (!Strings::TryParseFloat(line.value, light.common.intensity))
				{
					error = "Invalid rect light luminance";
					return false;
				}
			}
			else if (ParseCommonField(field, line, light.common, error, "LuminanceCdPerM2")) {}
			else if (!error.empty()) return false;
			else if (field == "Direction")
			{
				if (!Strings::TryParseFloat3(line.value, value.direction)) { error = "Invalid rect light direction"; return false; }
			}
			else if (field == "Tangent")
			{
				if (!Strings::TryParseFloat3(line.value, value.tangent)) { error = "Invalid rect light tangent"; return false; }
			}
			else if (field == "Width")
			{
				if (!Strings::TryParseFloat(line.value, value.width)) { error = "Invalid rect light width"; return false; }
			}
			else if (field == "Height")
			{
				if (!Strings::TryParseFloat(line.value, value.height)) { error = "Invalid rect light height"; return false; }
			}
			else if (field == "CastShadow")
			{
				if (!Strings::TryParseBool(line.value, value.castShadow)) { error = "Invalid rect light CastShadow value"; return false; }
			}
			else return false;
			light.payload = value;
			return true;
		}


	bool ParseLightField(
	    const ParsedLightFieldKey& key,
	    const ParsedLevelLine& parsedLine,
	    std::vector<SceneLightDesc>& lights,
	    std::string& errorMessage)
	{
		SceneLightDesc& light = ResolveLight(lights, key.Kind, key.Index);
		switch (key.Kind)
		{
			case SceneLightKind::Directional: return ParseDirectionalField(key.Field, parsedLine, light, errorMessage);
			case SceneLightKind::Point: return ParsePointField(key.Field, parsedLine, light, errorMessage);
			case SceneLightKind::Spot: return ParseSpotField(key.Field, parsedLine, light, errorMessage);
			case SceneLightKind::Rect: return ParseRectField(key.Field, parsedLine, light, errorMessage);
			case SceneLightKind::Unknown:
			default: return false;
		}
	}
}
