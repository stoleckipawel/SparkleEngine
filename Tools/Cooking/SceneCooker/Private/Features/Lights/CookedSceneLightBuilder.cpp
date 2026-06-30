#include "PCH.h"

#include "CookedSceneLightBuilder.h"

#include <algorithm>
#include <cstring>

namespace
{
	Assets::CookedSceneLightKind ToCookedLightKind(ImportedLightKind lightKind) noexcept
	{
		switch (lightKind)
		{
			case ImportedLightKind::Directional:
				return Assets::CookedSceneLightKind::Directional;
			case ImportedLightKind::Point:
				return Assets::CookedSceneLightKind::Point;
			case ImportedLightKind::Spot:
				return Assets::CookedSceneLightKind::Spot;
			case ImportedLightKind::Rect:
				return Assets::CookedSceneLightKind::Rect;
			case ImportedLightKind::Unknown:
			default:
				return Assets::CookedSceneLightKind::Unknown;
		}
	}

	Assets::CookedSceneLightRecord BuildLightRecord(const ImportedLight& importedLight)
	{
		Assets::CookedSceneLightRecord lightRecord;
		const std::size_t copyLength =
		    (std::min)(importedLight.name.size(), static_cast<std::size_t>(Assets::kCookedSceneLightNameCapacity - 1u));
		if (copyLength > 0)
		{
			std::memcpy(lightRecord.name, importedLight.name.data(), copyLength);
		}

		lightRecord.kind = ToCookedLightKind(importedLight.kind);
		lightRecord.worldTransform = importedLight.worldTransform;
		lightRecord.direction = importedLight.direction;
		lightRecord.color = importedLight.color;
		lightRecord.intensity = importedLight.intensity;
		lightRecord.range = importedLight.range;
		lightRecord.innerConeAngleRadians = importedLight.innerConeAngleRadians;
		lightRecord.outerConeAngleRadians = importedLight.outerConeAngleRadians;
		lightRecord.tangent = importedLight.tangent;
		lightRecord.width = importedLight.width;
		lightRecord.height = importedLight.height;
		lightRecord.sourceNodeIndex = importedLight.sourceNodeIndex;
		lightRecord.flags = importedLight.visible ? 1u : 0u;
		return lightRecord;
	}
}  // namespace

void CookedSceneLightBuilder::BuildLights(const SourceImportResult& importResult, CookedSceneBuild& outBuild)
{
	outBuild.manifest.lights.clear();
	outBuild.manifest.lights.reserve(importResult.scene.lights.size());

	for (const ImportedLight& importedLight : importResult.scene.lights)
	{
		outBuild.manifest.lights.push_back(BuildLightRecord(importedLight));
	}
}
