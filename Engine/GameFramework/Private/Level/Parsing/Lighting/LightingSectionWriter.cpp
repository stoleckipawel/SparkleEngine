#include "PCH.h"

#include "LightingSectionWriter.h"

#include <iomanip>

namespace LevelParsing
{

		void WriteCommonFields(
		    std::ofstream& output,
		    std::string_view prefix,
		    const SceneLightCommonDesc& common,
		    std::string_view intensityField)
		{
			if (!common.name.empty()) output << prefix << "Name = " << common.name << "\n";
			output << prefix << "Position = " << common.worldTransform._41 << ", " << common.worldTransform._42 << ", "
			       << common.worldTransform._43 << "\n";
			output << prefix << intensityField << " = " << common.intensity << "\n";
			output << prefix << "Color = " << common.color.x << ", " << common.color.y << ", " << common.color.z << "\n";
			output << prefix << "Visible = " << (common.visible ? "true" : "false") << "\n";
		}

		void WriteDirectional(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const SceneDirectionalLightDesc& value)
		{
			WriteCommonFields(output, prefix, light.common, "IntensityLux");
			output << prefix << "Direction = " << value.direction.x << ", " << value.direction.y << ", " << value.direction.z << "\n";
			output << prefix << "AngularDiameterRadians = " << value.angularDiameterRadians << "\n";
			output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
		}

		void WritePoint(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const PointLightDesc& value)
		{
			WriteCommonFields(output, prefix, light.common, "IntensityCandela");
			output << prefix << "Range = " << value.range << "\n";
			output << prefix << "SourceRadius = " << value.sourceRadius << "\n";
			output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
		}

		void WriteSpot(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const SpotLightDesc& value)
		{
			WriteCommonFields(output, prefix, light.common, "IntensityCandela");
			output << prefix << "Direction = " << value.direction.x << ", " << value.direction.y << ", " << value.direction.z << "\n";
			output << prefix << "Range = " << value.range << "\n";
			output << prefix << "SourceRadius = " << value.sourceRadius << "\n";
			output << prefix << "InnerConeAngleRadians = " << value.innerConeAngleRadians << "\n";
			output << prefix << "OuterConeAngleRadians = " << value.outerConeAngleRadians << "\n";
			output << prefix << "CastShadow = " << (value.castShadow ? "true" : "false") << "\n";
		}

		void WriteRect(std::ofstream& output, std::string_view prefix, const SceneLightDesc& light, const RectLightDesc& value)
		{
			WriteCommonFields(output, prefix, light.common, "LuminanceCdPerM2");
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
				WriteDirectional(output, "DirectionalLight" + std::to_string(directionalIndex++), light, *value);
				continue;
			}
			if (const PointLightDesc* value = light.GetPoint())
			{
				WritePoint(output, "PointLight" + std::to_string(pointIndex++), light, *value);
				continue;
			}
			if (const SpotLightDesc* value = light.GetSpot())
			{
				WriteSpot(output, "SpotLight" + std::to_string(spotIndex++), light, *value);
				continue;
			}
			if (const RectLightDesc* value = light.GetRect())
				WriteRect(output, "RectLight" + std::to_string(rectIndex++), light, *value);
		}
		output << "\n";
	}
}
