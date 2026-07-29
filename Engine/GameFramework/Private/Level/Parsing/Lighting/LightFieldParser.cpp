#include "PCH.h"

#include "LightFieldParser.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Strings/StringUtils.h"

#include <format>

static const auto g_lightFieldParserLogger = Logging::GetOrCreateLogger("GameFramework.LightFieldParser");

namespace LevelParsing
{
	class LightFieldParsing final
	{
	  public:
		static void Parse(
		    const ParsedLightFieldKey& key,
		    const ParsedLevelLine& parsedLine,
		    std::vector<SceneLightDesc>& lights);

	  private:
		static void SetLightPosition(SceneLightCommonDesc& common, const DirectX::XMFLOAT3& position) noexcept;
		static bool IsLightKind(const SceneLightDesc& light, SceneLightKind kind) noexcept;
		static std::string_view GetLightKindName(SceneLightKind kind) noexcept;
		static SceneLightDesc CreateLight(SceneLightKind kind);
		static SceneLightDesc& ResolveLight(std::vector<SceneLightDesc>& lights, SceneLightKind kind, std::size_t kindIndex);
		static bool IsCommonField(std::string_view field) noexcept;
		static void ParseCommonField(std::string_view field, const ParsedLevelLine& line, SceneLightCommonDesc& common);
		static void ParseDirectionalField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light);
		static void ParsePointField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light);
		static void ParseSpotField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light);
		static void ParseRectField(std::string_view field, const ParsedLevelLine& line, SceneLightDesc& light);
		[[noreturn]] static void UnsupportedField(SceneLightKind kind, std::string_view field);
	};

	void LightFieldParsing::SetLightPosition(SceneLightCommonDesc& common, const DirectX::XMFLOAT3& position) noexcept
	{
		common.worldTransform._41 = position.x;
		common.worldTransform._42 = position.y;
		common.worldTransform._43 = position.z;
	}

	bool LightFieldParsing::IsLightKind(const SceneLightDesc& light, SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return light.GetDirectional() != nullptr;
			case SceneLightKind::Point:
				return light.GetPoint() != nullptr;
			case SceneLightKind::Spot:
				return light.GetSpot() != nullptr;
			case SceneLightKind::Rect:
				return light.GetRect() != nullptr;
			case SceneLightKind::Unknown:
			default:
				return false;
		}
	}

	std::string_view LightFieldParsing::GetLightKindName(SceneLightKind kind) noexcept
	{
		switch (kind)
		{
			case SceneLightKind::Directional:
				return "directional";
			case SceneLightKind::Point:
				return "point";
			case SceneLightKind::Spot:
				return "spot";
			case SceneLightKind::Rect:
				return "rect";
			case SceneLightKind::Unknown:
			default:
				return "unknown";
		}
	}

	SceneLightDesc LightFieldParsing::CreateLight(SceneLightKind kind)
	{
		SceneLightDesc light;
		switch (kind)
		{
			case SceneLightKind::Directional:
				light.payload = SceneDirectionalLightDesc{};
				return light;
			case SceneLightKind::Point:
				light.payload = PointLightDesc{};
				return light;
			case SceneLightKind::Spot:
				light.payload = SpotLightDesc{};
				return light;
			case SceneLightKind::Rect:
				light.payload = RectLightDesc{};
				return light;
			case SceneLightKind::Unknown:
			default:
				Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Light field parser received an unknown light kind.");
		}
	}

	SceneLightDesc& LightFieldParsing::ResolveLight(
	    std::vector<SceneLightDesc>& lights,
	    SceneLightKind kind,
	    std::size_t kindIndex)
	{
		std::size_t currentKindIndex = 0;
		for (SceneLightDesc& light : lights)
		{
			if (!IsLightKind(light, kind))
				continue;
			if (currentKindIndex == kindIndex)
				return light;
			++currentKindIndex;
		}

		if (currentKindIndex != kindIndex)
			throw Diagnostics::Error("Light indices must be contiguous for each light kind.");
		lights.push_back(CreateLight(kind));
		return lights.back();
	}

	bool LightFieldParsing::IsCommonField(std::string_view field) noexcept
	{
		return field == "Name" || field == "Color" || field == "Visible" || field == "Position";
	}

	void LightFieldParsing::ParseCommonField(
	    std::string_view field,
	    const ParsedLevelLine& line,
	    SceneLightCommonDesc& common)
	{
		if (field == "Name")
		{
			common.name = Strings::UnquoteCopy(line.value);
			if (common.name.empty())
				throw Diagnostics::Error("Light name cannot be empty.");
			return;
		}
		if (field == "Color")
		{
			common.color = ParseFloat3(line.value, "light color");
			return;
		}
		if (field == "Visible")
		{
			common.visible = ParseBool(line.value, "light visibility");
			return;
		}
		if (field == "Position")
		{
			SetLightPosition(common, ParseFloat3(line.value, "light position"));
			return;
		}
		Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Common light field parser received another field kind.");
	}

	void LightFieldParsing::ParseDirectionalField(
	    std::string_view field,
	    const ParsedLevelLine& line,
	    SceneLightDesc& light)
	{
		const SceneDirectionalLightDesc* current = light.GetDirectional();
		if (current == nullptr)
			Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Directional light field resolved to another light kind.");
		SceneDirectionalLightDesc value = *current;
		if (IsCommonField(field))
			ParseCommonField(field, line, light.common);
		else if (field == "Direction")
			value.direction = ParseFloat3(line.value, "directional light direction");
		else if (field == "CastShadow")
			value.castShadow = ParseBool(line.value, "directional light shadow flag");
		else if (field == "IlluminanceLux")
			value.illuminance = ParseFloat(line.value, "directional light illuminance");
		else if (field == "AngularSizeRadians")
			value.angularSizeRadians = ParseFloat(line.value, "directional light angular size");
		else
			UnsupportedField(SceneLightKind::Directional, field);
		light.payload = value;
	}

	void LightFieldParsing::ParsePointField(
	    std::string_view field,
	    const ParsedLevelLine& line,
	    SceneLightDesc& light)
	{
		const PointLightDesc* current = light.GetPoint();
		if (current == nullptr)
			Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Point light field resolved to another light kind.");
		PointLightDesc value = *current;
		if (IsCommonField(field))
			ParseCommonField(field, line, light.common);
		else if (field == "LuminousIntensityCandela")
			value.luminousIntensity = ParseFloat(line.value, "point light luminous intensity");
		else if (field == "Range")
			value.range = ParseFloat(line.value, "point light range");
		else if (field == "Radius")
			value.radius = ParseFloat(line.value, "point light radius");
		else if (field == "DistanceAttenuationCoefficients")
			value.distanceAttenuationCoefficients = ParseFloat3(line.value, "point light distance attenuation coefficients");
		else if (field == "CastShadow")
			value.castShadow = ParseBool(line.value, "point light shadow flag");
		else
			UnsupportedField(SceneLightKind::Point, field);
		light.payload = value;
	}

	void LightFieldParsing::ParseSpotField(
	    std::string_view field,
	    const ParsedLevelLine& line,
	    SceneLightDesc& light)
	{
		const SpotLightDesc* current = light.GetSpot();
		if (current == nullptr)
			Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Spot light field resolved to another light kind.");
		SpotLightDesc value = *current;
		if (IsCommonField(field))
			ParseCommonField(field, line, light.common);
		else if (field == "Direction")
			value.direction = ParseFloat3(line.value, "spot light direction");
		else if (field == "LuminousIntensityCandela")
			value.luminousIntensity = ParseFloat(line.value, "spot light luminous intensity");
		else if (field == "Range")
			value.range = ParseFloat(line.value, "spot light range");
		else if (field == "Radius")
			value.radius = ParseFloat(line.value, "spot light radius");
		else if (field == "DistanceAttenuationCoefficients")
			value.distanceAttenuationCoefficients = ParseFloat3(line.value, "spot light distance attenuation coefficients");
		else if (field == "InnerAngleRadians")
			value.innerAngleRadians = ParseFloat(line.value, "spot light inner angle");
		else if (field == "OuterAngleRadians")
			value.outerAngleRadians = ParseFloat(line.value, "spot light outer angle");
		else if (field == "CastShadow")
			value.castShadow = ParseBool(line.value, "spot light shadow flag");
		else
			UnsupportedField(SceneLightKind::Spot, field);
		light.payload = value;
	}

	void LightFieldParsing::ParseRectField(
	    std::string_view field,
	    const ParsedLevelLine& line,
	    SceneLightDesc& light)
	{
		const RectLightDesc* current = light.GetRect();
		if (current == nullptr)
			Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Rect light field resolved to another light kind.");
		RectLightDesc value = *current;
		if (IsCommonField(field))
			ParseCommonField(field, line, light.common);
		else if (field == "LuminanceCdPerM2")
			value.luminance = ParseFloat(line.value, "rect light luminance");
		else if (field == "Direction")
			value.direction = ParseFloat3(line.value, "rect light direction");
		else if (field == "Tangent")
			value.tangent = ParseFloat3(line.value, "rect light tangent");
		else if (field == "Width")
			value.width = ParseFloat(line.value, "rect light width");
		else if (field == "Height")
			value.height = ParseFloat(line.value, "rect light height");
		else if (field == "CastShadow")
			value.castShadow = ParseBool(line.value, "rect light shadow flag");
		else
			UnsupportedField(SceneLightKind::Rect, field);
		light.payload = value;
	}

	[[noreturn]] void LightFieldParsing::UnsupportedField(SceneLightKind kind, std::string_view field)
	{
		throw Diagnostics::Error(std::format("Unsupported {} light field '{}'.", GetLightKindName(kind), field));
	}

	void LightFieldParsing::Parse(
	    const ParsedLightFieldKey& key,
	    const ParsedLevelLine& parsedLine,
	    std::vector<SceneLightDesc>& lights)
	{
		SceneLightDesc& light = ResolveLight(lights, key.Kind, key.Index);
		switch (key.Kind)
		{
			case SceneLightKind::Directional:
				ParseDirectionalField(key.Field, parsedLine, light);
				return;
			case SceneLightKind::Point:
				ParsePointField(key.Field, parsedLine, light);
				return;
			case SceneLightKind::Spot:
				ParseSpotField(key.Field, parsedLine, light);
				return;
			case SceneLightKind::Rect:
				ParseRectField(key.Field, parsedLine, light);
				return;
			case SceneLightKind::Unknown:
			default:
				Diagnostics::Fatal(g_lightFieldParserLogger, __FILE__, __LINE__, "Light field parser received an unknown light kind.");
		}
	}

	void ParseLightField(
	    const ParsedLightFieldKey& key,
	    const ParsedLevelLine& parsedLine,
	    std::vector<SceneLightDesc>& lights)
	{
		LightFieldParsing::Parse(key, parsedLine, lights);
	}
}
