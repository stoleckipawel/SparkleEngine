#include "PCH.h"

#include "CookedSceneLightBuilder.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cstring>
#include <format>

class CookedLightTranslation final
{
  public:
	static Assets::CookedSceneLightKind ToCookedLightKind(ImportedLightKind lightKind) noexcept
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

	static Assets::CookedSceneLightRecord BuildLightRecord(const ImportedLight& importedLight)
	{
		Assets::CookedSceneLightRecord lightRecord;
		if (!importedLight.name.empty())
		{
			std::memcpy(lightRecord.name, importedLight.name.data(), importedLight.name.size());
		}

		lightRecord.kind = ToCookedLightKind(importedLight.kind);
		lightRecord.worldTransform = importedLight.worldTransform;
		lightRecord.direction = importedLight.direction;
		lightRecord.color = importedLight.color;
		lightRecord.illuminance = importedLight.illuminance;
		lightRecord.luminousIntensity = importedLight.luminousIntensity;
		lightRecord.luminance = importedLight.luminance;
		lightRecord.range = importedLight.range;
		lightRecord.distanceAttenuationCoefficients = importedLight.distanceAttenuationCoefficients;
		lightRecord.radius = importedLight.radius;
		lightRecord.innerAngleRadians = importedLight.innerAngleRadians;
		lightRecord.outerAngleRadians = importedLight.outerAngleRadians;
		lightRecord.angularSizeRadians = importedLight.angularSizeRadians;
		lightRecord.tangent = importedLight.tangent;
		lightRecord.width = importedLight.width;
		lightRecord.height = importedLight.height;
		lightRecord.sourceNodeIndex = importedLight.sourceNodeIndex;
		lightRecord.flags = importedLight.visible ? 1u : 0u;
		return lightRecord;
	}
};

void CookedSceneLightBuilder::BuildLights(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild)
{
	outBuild.manifest.lights.clear();
	outBuild.manifest.lights.reserve(importOutput.scene.lights.size());

	for (std::size_t lightIndex = 0; lightIndex < importOutput.scene.lights.size(); ++lightIndex)
	{
		const ImportedLight& importedLight = importOutput.scene.lights[lightIndex];
		if (importedLight.name.size() >= Assets::kCookedSceneLightNameCapacity ||
		    CookedLightTranslation::ToCookedLightKind(importedLight.kind) == Assets::CookedSceneLightKind::Unknown)
		{
			throw Diagnostics::Error(std::format("Imported light {} exceeds the cooked light contract.", lightIndex));
		}
		outBuild.manifest.lights.push_back(CookedLightTranslation::BuildLightRecord(importedLight));
	}
}
