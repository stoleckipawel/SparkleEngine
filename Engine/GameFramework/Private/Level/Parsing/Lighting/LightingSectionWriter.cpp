#include "PCH.h"

#include "LightingSectionWriter.h"

#include "Core/Public/Diagnostics/Verify.h"

#include <iomanip>

static const auto g_lightingSectionWriterLogger = Logging::GetOrCreateLogger("GameFramework.LevelParsing");

namespace LevelParsing
{
	class LightingSectionSerialization final
	{
	  public:
		static void WriteDirectional(
		    std::ofstream& output,
		    std::string_view prefix,
		    const SceneLightDesc& light,
		    const SceneDirectionalLightDesc& value);
		static void WritePoint(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const PointLightDesc& value);
		static void WriteSpot(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const SpotLightDesc& value);
		static void WriteRect(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const RectLightDesc& value);

	  private:
		static void WriteCommonFields(
		    std::ofstream& output,
		    std::string_view prefix,
		    const SceneLightCommonDesc& common);
	};

	void LightingSectionSerialization::WriteCommonFields(
	    std::ofstream& output,
	    std::string_view prefix,
	    const SceneLightCommonDesc& common)
	{
		if (!common.name.empty())
			output << prefix << "Name = " << common.name << "\n";
		output << prefix << "Position = " << common.worldTransform._41 << ", " << common.worldTransform._42 << ", "
		       << common.worldTransform._43 << "\n";
		output << prefix << "Color = " << common.color.x << ", " << common.color.y << ", " << common.color.z << "\n";
		output << prefix << "Visible = " << (common.visible ? "true" : "false") << "\n";
	}

	void LightingSectionSerialization::WriteDirectional(
	    std::ofstream& output,
	    std::string_view prefix,
	    const SceneLightDesc& light,
	    const SceneDirectionalLightDesc& value)
	{
		WriteCommonFields(output, prefix, light.common);
		output << prefix << "IlluminanceLux = " << value.illuminance << "\n";
		output << prefix << "Direction = " << value.direction.x << ", " << value.direction.y << ", " << value.direction.z << "\n";
		output << prefix << "AngularSizeRadians = " << value.angularSizeRadians << "\n";
		output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
	}

	void LightingSectionSerialization::WritePoint(
	    std::ofstream& output,
	    std::string_view prefix,
	    const SceneLightDesc& light,
	    const PointLightDesc& value)
	{
		WriteCommonFields(output, prefix, light.common);
		output << prefix << "LuminousIntensityCandela = " << value.luminousIntensity << "\n";
		output << prefix << "Range = " << value.range << "\n";
		output << prefix << "Radius = " << value.radius << "\n";
		output << prefix << "DistanceAttenuationCoefficients = " << value.distanceAttenuationCoefficients.x << ", "
		       << value.distanceAttenuationCoefficients.y << ", " << value.distanceAttenuationCoefficients.z << "\n";
		output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
	}

	void LightingSectionSerialization::WriteSpot(
	    std::ofstream& output,
	    std::string_view prefix,
	    const SceneLightDesc& light,
	    const SpotLightDesc& value)
	{
		WriteCommonFields(output, prefix, light.common);
		output << prefix << "LuminousIntensityCandela = " << value.luminousIntensity << "\n";
		output << prefix << "Direction = " << value.direction.x << ", " << value.direction.y << ", " << value.direction.z << "\n";
		output << prefix << "Range = " << value.range << "\n";
		output << prefix << "Radius = " << value.radius << "\n";
		output << prefix << "DistanceAttenuationCoefficients = " << value.distanceAttenuationCoefficients.x << ", "
		       << value.distanceAttenuationCoefficients.y << ", " << value.distanceAttenuationCoefficients.z << "\n";
		output << prefix << "InnerAngleRadians = " << value.innerAngleRadians << "\n";
		output << prefix << "OuterAngleRadians = " << value.outerAngleRadians << "\n";
		output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
	}

	void LightingSectionSerialization::WriteRect(
	    std::ofstream& output,
	    std::string_view prefix,
	    const SceneLightDesc& light,
	    const RectLightDesc& value)
	{
		WriteCommonFields(output, prefix, light.common);
		output << prefix << "LuminanceCdPerM2 = " << value.luminance << "\n";
		output << prefix << "Direction = " << value.direction.x << ", " << value.direction.y << ", " << value.direction.z << "\n";
		output << prefix << "Tangent = " << value.tangent.x << ", " << value.tangent.y << ", " << value.tangent.z << "\n";
		output << prefix << "Width = " << value.width << "\n";
		output << prefix << "Height = " << value.height << "\n";
		output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
	}


	void WriteLightingSectionValues(std::ofstream& output, const LevelDesc& levelDesc)
	{
		output << std::setprecision(9);
		output << "[Lighting]\n";
		std::size_t directionalIndex = 0;
		std::size_t pointIndex = 0;
		std::size_t spotIndex = 0;
		std::size_t rectIndex = 0;
		for (const SceneLightDesc& light : levelDesc.lights)
		{
			if (const SceneDirectionalLightDesc* value = light.GetDirectional())
			{
				LightingSectionSerialization::WriteDirectional(
				    output,
				    "DirectionalLight" + std::to_string(directionalIndex++),
				    light,
				    *value);
				continue;
			}
			if (const PointLightDesc* value = light.GetPoint())
			{
				LightingSectionSerialization::WritePoint(output, "PointLight" + std::to_string(pointIndex++), light, *value);
				continue;
			}
			if (const SpotLightDesc* value = light.GetSpot())
			{
				LightingSectionSerialization::WriteSpot(output, "SpotLight" + std::to_string(spotIndex++), light, *value);
				continue;
			}
			if (const RectLightDesc* value = light.GetRect())
			{
				LightingSectionSerialization::WriteRect(output, "RectLight" + std::to_string(rectIndex++), light, *value);
				continue;
			}
			Diagnostics::Fatal(g_lightingSectionWriterLogger, __FILE__, __LINE__, "Level contains an unsupported light payload.");
		}
		output << "\n";
	}
}
