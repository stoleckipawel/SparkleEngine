#include "PCH.h"

#include "SceneAssetLightTranslator.h"

#include "Core/Public/Diagnostics/Verify.h"

#include <string>

static const auto g_sceneAssetLightTranslatorLogger = Logging::GetOrCreateLogger("GameFramework.SceneAssetLightTranslator");

namespace Assets
{
	class SceneAssetLightTranslation final
	{
	public:
		static SceneLightPayload BuildPayload(const CookedSceneLightRecord& lightRecord)
		{
			switch (lightRecord.kind)
			{
				case CookedSceneLightKind::Directional:
				{
					SceneDirectionalLightDesc directional;
					directional.direction = lightRecord.direction;
					directional.illuminance = lightRecord.illuminance;
					directional.angularSizeRadians = lightRecord.angularSizeRadians;
					return directional;
				}
				case CookedSceneLightKind::Point:
				{
					PointLightDesc point;
					point.luminousIntensity = lightRecord.luminousIntensity;
					point.range = lightRecord.range;
					point.radius = lightRecord.radius;
					point.distanceAttenuationCoefficients = lightRecord.distanceAttenuationCoefficients;
					point.castShadow = true;
					return point;
				}
				case CookedSceneLightKind::Spot:
				{
					SpotLightDesc spot;
					spot.direction = lightRecord.direction;
					spot.luminousIntensity = lightRecord.luminousIntensity;
					spot.range = lightRecord.range;
					spot.radius = lightRecord.radius;
					spot.distanceAttenuationCoefficients = lightRecord.distanceAttenuationCoefficients;
					spot.innerAngleRadians = lightRecord.innerAngleRadians;
					spot.outerAngleRadians = lightRecord.outerAngleRadians;
					spot.castShadow = true;
					return spot;
				}
				case CookedSceneLightKind::Rect:
				{
					RectLightDesc rect;
					rect.direction = lightRecord.direction;
					rect.luminance = lightRecord.luminance;
					rect.tangent = lightRecord.tangent;
					rect.width = lightRecord.width;
					rect.height = lightRecord.height;
					rect.castShadow = true;
					return rect;
				}
				case CookedSceneLightKind::Unknown:
				default:
					Diagnostics::Fatal(
					    g_sceneAssetLightTranslatorLogger,
					    __FILE__,
					    __LINE__,
					    "Validated cooked scene contains an unsupported light kind.");
			}
		}
	};

	SceneLightDesc BuildSceneAssetLight(const CookedSceneLightRecord& lightRecord)
	{
		SceneLightDesc light;
		light.common.name = lightRecord.name;
		light.common.worldTransform = lightRecord.worldTransform;
		light.common.color = lightRecord.color;
		light.common.visible = (lightRecord.flags & 1u) != 0u;
		light.payload = SceneAssetLightTranslation::BuildPayload(lightRecord);
		return light;
	}
}
