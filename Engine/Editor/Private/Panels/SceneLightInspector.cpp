#include "PCH.h"
#include "Panels/SceneLightInspector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/GameScene.h"
#include "Scene/Lighting/PointLightDesc.h"
#include "Scene/Lighting/SceneDirectionalLightDesc.h"
#include "Scene/Lighting/SceneLightDesc.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Lighting/SpotLightDesc.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <utility>

namespace
{
	constexpr float kAngularDiameterDragSpeedDegrees = 0.1f;
	constexpr float kAngularDiameterSliderMaxDegrees = 30.0f;
	constexpr float kSourceRadiusSliderMax = 25.0f;
}

void SceneLightInspector::Build(GameScene& gameScene, std::size_t lightIndex, const std::string& filterText) noexcept
{
	const SceneLightDesc* sceneLight = gameScene.GetLighting().GetLight(lightIndex);
	if (sceneLight == nullptr)
	{
		UiUtil::DrawDetailsEmptyState();
		return;
	}

	BuildGenericLight(gameScene, lightIndex, *sceneLight, filterText);
}

void SceneLightInspector::BuildGenericLight(
    GameScene& gameScene,
    std::size_t lightIndex,
    const SceneLightDesc& sceneLight,
    const std::string& filterText) noexcept
{
	SceneLightDesc lightDesc = sceneLight;
	BuildLightCommonCategory(filterText, lightDesc);

	if (SceneDirectionalLightDesc* directional = lightDesc.GetDirectional())
	{
		BuildDirectionalLightTransformCategory(filterText, *directional);
		BuildDirectionalLightCategory(filterText, *directional);
	}
	else if (PointLightDesc* point = lightDesc.GetPoint())
	{
		BuildPointLightCategory(filterText, *point);
	}
	else if (SpotLightDesc* spot = lightDesc.GetSpot())
	{
		BuildSpotLightCategory(filterText, *spot);
	}

	gameScene.GetLighting().ApplyLightDesc(lightIndex, std::move(lightDesc));
}

void SceneLightInspector::BuildLightCommonCategory(const std::string& filterText, SceneLightDesc& lightDesc) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Light", "intensity lux candela color visible visibility rendering"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Light"))
	{
		return;
	}

	float intensity = lightDesc.common.intensity;
	constexpr float kDefaultIntensity = 1.0f;
	const bool isDirectionalLight = lightDesc.GetDirectional() != nullptr;
	const char* intensityLabel = isDirectionalLight ? "Intensity (lux)" : "Intensity (candela)";
	const float intensityDragSpeed = isDirectionalLight ? 100.0f : 10.0f;
	if (UiUtil::EditDetailsFloat(
	        intensityLabel,
	        intensity,
	        intensityDragSpeed,
	        0.0f,
	        0.0f,
	        "%.3f",
	        &kDefaultIntensity))
	{
		lightDesc.common.intensity = (std::max) (0.0f, intensity);
	}

	float colorValues[3] = {lightDesc.common.color.x, lightDesc.common.color.y, lightDesc.common.color.z};
	const float defaultColor[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsColor3("Color", colorValues, defaultColor))
	{
		lightDesc.common.color = {colorValues[0], colorValues[1], colorValues[2]};
	}

	constexpr bool kDefaultVisible = true;
	bool visible = lightDesc.common.visible;
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &kDefaultVisible))
	{
		lightDesc.common.visible = visible;
	}

	UiUtil::EndDetailsCategory();
}

void SceneLightInspector::BuildDirectionalLightTransformCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Transform", "rotation direction transform"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Transform"))
	{
		return;
	}

	DirectX::XMFLOAT3 rotationDegrees = MathUtils::DirectionToRotationDegrees(lightDesc.direction);
	float rotationValues[3] = {rotationDegrees.x, rotationDegrees.y, rotationDegrees.z};
	const DirectX::XMFLOAT3 defaultRotation = MathUtils::DirectionToRotationDegrees(DirectX::XMFLOAT3{0.0f, -1.0f, 0.0f});
	const float defaultRotationValues[3] = {defaultRotation.x, defaultRotation.y, defaultRotation.z};
	if (UiUtil::EditDetailsFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f", defaultRotationValues))
	{
		lightDesc.direction =
		    MathUtils::RotationDegreesToDirection(DirectX::XMFLOAT3{rotationValues[0], rotationValues[1], rotationValues[2]});
	}

	UiUtil::EndDetailsCategory();
}

void SceneLightInspector::BuildDirectionalLightCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Directional Light", "direction cast shadow rendering"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Directional Light"))
	{
		return;
	}

	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	const float defaultDirection[3] = {0.0f, -1.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Direction", directionValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultDirection))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
	}

	bool castShadow = lightDesc.castShadow;
	constexpr bool kDefaultCastShadow = true;
	if (UiUtil::EditDetailsCheckbox("Cast Shadow", castShadow, &kDefaultCastShadow))
	{
		lightDesc.castShadow = castShadow;
	}

	float angularDiameterDegrees = MathUtils::RadiansToDegrees(lightDesc.angularDiameterRadians);
	constexpr float kDefaultAngularDiameterDegrees = 0.533f;
	if (UiUtil::EditDetailsFloat(
	        "Angular Diameter",
	        angularDiameterDegrees,
	        kAngularDiameterDragSpeedDegrees,
	        0.0f,
	        kAngularDiameterSliderMaxDegrees,
	        "%.3f",
	        &kDefaultAngularDiameterDegrees))
	{
		lightDesc.angularDiameterRadians = MathUtils::DegreesToRadians((std::max) (0.0f, angularDiameterDegrees));
	}
	UiUtil::EndDetailsCategory();
}

void SceneLightInspector::BuildPointLightCategory(const std::string& filterText, PointLightDesc& lightDesc) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Point Light", "range radius attenuation point"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Point Light"))
	{
		return;
	}

	float range = lightDesc.range;
	constexpr float kDefaultRange = 0.0f;
	if (UiUtil::EditDetailsFloat("Range", range, 0.05f, 0.0f, kRangeSliderMax, "%.3f", &kDefaultRange))
	{
		lightDesc.range = (std::max) (0.0f, range);
	}

	float sourceRadius = lightDesc.sourceRadius;
	constexpr float kDefaultSourceRadius = 0.05f;
	if (UiUtil::EditDetailsFloat(
	        "Source Radius",
	        sourceRadius,
	        0.01f,
	        0.0f,
	        kSourceRadiusSliderMax,
	        "%.3f",
	        &kDefaultSourceRadius))
	{
		lightDesc.sourceRadius = (std::max) (0.0f, sourceRadius);
	}

	UiUtil::EndDetailsCategory();
}

void SceneLightInspector::BuildSpotLightCategory(const std::string& filterText, SpotLightDesc& lightDesc) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Spot Light", "direction range cone angle spot"))
	{
		return;
	}

	if (!UiUtil::BeginDetailsCategory("Spot Light"))
	{
		return;
	}

	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	const float defaultDirection[3] = {0.0f, -1.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Direction", directionValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultDirection))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
	}

	float range = lightDesc.range;
	constexpr float kDefaultRange = 0.0f;
	if (UiUtil::EditDetailsFloat("Range", range, 0.05f, 0.0f, kRangeSliderMax, "%.3f", &kDefaultRange))
	{
		lightDesc.range = (std::max) (0.0f, range);
	}

	float sourceRadius = lightDesc.sourceRadius;
	constexpr float kDefaultSourceRadius = 0.05f;
	if (UiUtil::EditDetailsFloat(
	        "Source Radius",
	        sourceRadius,
	        0.01f,
	        0.0f,
	        kSourceRadiusSliderMax,
	        "%.3f",
	        &kDefaultSourceRadius))
	{
		lightDesc.sourceRadius = (std::max) (0.0f, sourceRadius);
	}

	float innerConeDegrees = MathUtils::RadiansToDegrees(lightDesc.innerConeAngleRadians);
	constexpr float kDefaultInnerConeDegrees = 0.0f;
	if (UiUtil::EditDetailsFloat("Inner Cone", innerConeDegrees, 0.1f, 0.0f, 180.0f, "%.2f", &kDefaultInnerConeDegrees))
	{
		lightDesc.innerConeAngleRadians = MathUtils::DegreesToRadians((std::max) (0.0f, innerConeDegrees));
	}

	float outerConeDegrees = MathUtils::RadiansToDegrees(lightDesc.outerConeAngleRadians);
	constexpr float kDefaultOuterConeDegrees = 45.0f;
	if (UiUtil::EditDetailsFloat("Outer Cone", outerConeDegrees, 0.1f, 0.0f, 180.0f, "%.2f", &kDefaultOuterConeDegrees))
	{
		lightDesc.outerConeAngleRadians = MathUtils::DegreesToRadians((std::max) (0.0f, outerConeDegrees));
	}

	UiUtil::EndDetailsCategory();
}
