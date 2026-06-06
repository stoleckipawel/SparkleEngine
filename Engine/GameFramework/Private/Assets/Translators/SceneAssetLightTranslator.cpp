#include "PCH.h"

#include "SceneAssetLightTranslator.h"

#include <string>

namespace Assets
{
	namespace
	{
		SceneLightKind ToSceneLightKind(CookedSceneLightKind lightKind) noexcept
		{
			switch (lightKind)
			{
				case CookedSceneLightKind::Directional:
					return SceneLightKind::Directional;
				case CookedSceneLightKind::Point:
					return SceneLightKind::Point;
				case CookedSceneLightKind::Spot:
					return SceneLightKind::Spot;
				case CookedSceneLightKind::Unknown:
				default:
					return SceneLightKind::Unknown;
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
		light.name = CookedLightNameToString(lightRecord, lightIndex);
		light.kind = ToSceneLightKind(lightRecord.kind);
		light.directional.direction = lightRecord.direction;
		light.directional.color = lightRecord.color;
		light.directional.intensity = lightRecord.intensity;
		light.worldTransform = lightRecord.worldTransform;
		light.range = lightRecord.range;
		light.innerConeAngleRadians = lightRecord.innerConeAngleRadians;
		light.outerConeAngleRadians = lightRecord.outerConeAngleRadians;
		light.visible = (lightRecord.flags & 1u) != 0u;
		return light;
	}
}  // namespace Assets
