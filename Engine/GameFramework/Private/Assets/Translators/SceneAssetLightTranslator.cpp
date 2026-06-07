#include "PCH.h"

#include "SceneAssetLightTranslator.h"

#include <string>

namespace Assets
{
	namespace
	{
		SceneLightPayload BuildSceneLightPayload(const CookedSceneLightRecord& lightRecord)
		{
			switch (lightRecord.kind)
			{
				case CookedSceneLightKind::Directional:
				{
					SceneDirectionalLightDesc directional;
					directional.direction = lightRecord.direction;
					return directional;
				}
				case CookedSceneLightKind::Point:
				{
					PointLightDesc point;
					point.range = lightRecord.range;
					point.sourceRadius = 0.05f;
					point.castShadow = true;
					return point;
				}
				case CookedSceneLightKind::Spot:
				{
					SpotLightDesc spot;
					spot.direction = lightRecord.direction;
					spot.range = lightRecord.range;
					spot.sourceRadius = 0.05f;
					spot.innerConeAngleRadians = lightRecord.innerConeAngleRadians;
					spot.outerConeAngleRadians = lightRecord.outerConeAngleRadians;
					spot.castShadow = true;
					return spot;
				}
				case CookedSceneLightKind::Unknown:
				default:
					return std::monostate{};
			}
		}

		std::string CookedLightNameToString(const CookedSceneLightRecord& lightRecord, std::size_t lightIndex)
		{
			std::size_t length = 0;
			while (length < kCookedSceneLightNameCapacity && lightRecord.name[length] != '\0')
			{
				++length;
			}

			if (length == 0)
			{
				return "Light " + std::to_string(lightIndex + 1);
			}

			return std::string(lightRecord.name, length);
		}
	}  // namespace

	SceneLightDesc BuildSceneAssetLight(const CookedSceneLightRecord& lightRecord, std::size_t lightIndex)
	{
		SceneLightDesc light;
		light.common.name = CookedLightNameToString(lightRecord, lightIndex);
		light.common.worldTransform = lightRecord.worldTransform;
		light.common.color = lightRecord.color;
		light.common.intensity = lightRecord.intensity;
		light.common.visible = (lightRecord.flags & 1u) != 0u;
		light.payload = BuildSceneLightPayload(lightRecord);
		return light;
	}
}  // namespace Assets
