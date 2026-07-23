#include "PCH.h"
#include "Panels/SceneLightInspector.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/Transactions/EditorTransactionManager.h"
#include "Scene/Lighting/PointLightDesc.h"
#include "Scene/Lighting/RectLightDesc.h"
#include "Scene/Lighting/SceneDirectionalLightDesc.h"
#include "Scene/Lighting/SceneLightDesc.h"
#include "Scene/Lighting/SpotLightDesc.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <utility>

class SceneLightInspectorConstants final
{
  public:
	static constexpr float kAngularDiameterDragSpeedDegrees = 0.1f;
	static constexpr float kAngularDiameterSliderMaxDegrees = 30.0f;
	static constexpr float kSourceRadiusSliderMax = 25.0f;
	static constexpr float kAreaLightSizeSliderMax = 100.0f;
};

void SceneLightInspector::Build(const SceneLightDesc& sceneLight, EntityId lightEntity,
                                EditorTransactionManager& transactions, std::uint64_t worldGeneration,
                                const std::string& filterText) noexcept
{
	BuildGenericLight(transactions, worldGeneration, lightEntity, sceneLight, filterText);
}

void SceneLightInspector::BuildGenericLight(
    EditorTransactionManager& transactions,
    std::uint64_t worldGeneration,
    EntityId lightEntity,
    const SceneLightDesc& sceneLight,
	const std::string& filterText) noexcept
{
	SceneLightDesc lightDesc = sceneLight;
	bool changed = BuildLightCommonCategory(filterText, lightDesc);

	if (SceneDirectionalLightDesc* directional = lightDesc.GetDirectional())
	{
		changed |= BuildDirectionalLightTransformCategory(filterText, *directional);
		changed |= BuildDirectionalLightCategory(filterText, *directional);
	}
	else if (PointLightDesc* point = lightDesc.GetPoint())
	{
		changed |= BuildPointLightCategory(filterText, *point);
	}
	else if (SpotLightDesc* spot = lightDesc.GetSpot())
	{
		changed |= BuildSpotLightCategory(filterText, *spot);
	}
	else if (RectLightDesc* rect = lightDesc.GetRect())
	{
		changed |= BuildRectLightCategory(filterText, *rect);
	}

	if (!changed) return;
	(void) transactions.Execute(
	    {0, SetLightDescriptionCommand{lightEntity, std::move(lightDesc)}},
	    {0, SetLightDescriptionCommand{lightEntity, sceneLight}},
	    worldGeneration,
	    "light-description");
}

bool SceneLightInspector::BuildLightCommonCategory(const std::string& filterText, SceneLightDesc& lightDesc) noexcept
{
	bool changed = false;
	if (!UiUtil::MatchesDetailsFilter(filterText, "Light", "intensity lux candela luminance color visible visibility rendering"))
	{
		return false;
	}

	if (!UiUtil::BeginDetailsCategory("Light"))
	{
		return false;
	}

	float intensity = lightDesc.common.intensity;
	constexpr float kDefaultIntensity = 1.0f;
	const bool isDirectionalLight = lightDesc.GetDirectional() != nullptr;
	const bool isRectLight = lightDesc.GetRect() != nullptr;
	const char* intensityLabel =
	    isDirectionalLight ? "Intensity (lux)" : (isRectLight ? "Luminance (cd/m^2)" : "Intensity (candela)");
	const float intensityDragSpeed = isDirectionalLight ? 100.0f : (isRectLight ? 10.0f : 10.0f);
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
		changed = true;
	}

	float colorValues[3] = {lightDesc.common.color.x, lightDesc.common.color.y, lightDesc.common.color.z};
	const float defaultColor[3] = {1.0f, 1.0f, 1.0f};
	if (UiUtil::EditDetailsColor3("Color", colorValues, defaultColor))
	{
		lightDesc.common.color = {colorValues[0], colorValues[1], colorValues[2]};
		changed = true;
	}

	constexpr bool kDefaultVisible = true;
	bool visible = lightDesc.common.visible;
	if (UiUtil::EditDetailsCheckbox("Visible", visible, &kDefaultVisible))
	{
		lightDesc.common.visible = visible;
		changed = true;
	}

	UiUtil::EndDetailsCategory();
	return changed;
}

bool SceneLightInspector::BuildDirectionalLightTransformCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept
{
	if (!UiUtil::MatchesDetailsFilter(filterText, "Transform", "rotation direction transform"))
	{
		return false;
	}

	if (!UiUtil::BeginDetailsCategory("Transform"))
	{
		return false;
	}

	DirectX::XMFLOAT3 rotationDegrees = MathUtils::DirectionToRotationDegrees(lightDesc.direction);
	float rotationValues[3] = {rotationDegrees.x, rotationDegrees.y, rotationDegrees.z};
	const DirectX::XMFLOAT3 defaultRotation = MathUtils::DirectionToRotationDegrees(DirectX::XMFLOAT3{0.0f, -1.0f, 0.0f});
	const float defaultRotationValues[3] = {defaultRotation.x, defaultRotation.y, defaultRotation.z};
	if (UiUtil::EditDetailsFloat3("Rotation", rotationValues, 0.1f, -360.0f, 360.0f, "%.2f", defaultRotationValues))
	{
		lightDesc.direction =
		    MathUtils::RotationDegreesToDirection(DirectX::XMFLOAT3{rotationValues[0], rotationValues[1], rotationValues[2]});
		UiUtil::EndDetailsCategory();
		return true;
	}

	UiUtil::EndDetailsCategory();
	return false;
}

bool SceneLightInspector::BuildDirectionalLightCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept
{
	bool changed = false;
	if (!UiUtil::MatchesDetailsFilter(filterText, "Directional Light", "direction cast shadow rendering"))
	{
		return false;
	}

	if (!UiUtil::BeginDetailsCategory("Directional Light"))
	{
		return false;
	}

	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	const float defaultDirection[3] = {0.0f, -1.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Direction", directionValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultDirection))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
		changed = true;
	}

	bool castShadow = lightDesc.castShadow;
	constexpr bool kDefaultCastShadow = true;
	if (UiUtil::EditDetailsCheckbox("Cast Shadow", castShadow, &kDefaultCastShadow))
	{
		lightDesc.castShadow = castShadow;
		changed = true;
	}

	float angularDiameterDegrees = MathUtils::RadiansToDegrees(lightDesc.angularDiameterRadians);
	constexpr float kDefaultAngularDiameterDegrees = 0.533f;
	if (UiUtil::EditDetailsFloat(
	        "Angular Diameter",
	        angularDiameterDegrees,
	        SceneLightInspectorConstants::kAngularDiameterDragSpeedDegrees,
	        0.0f,
	        SceneLightInspectorConstants::kAngularDiameterSliderMaxDegrees,
	        "%.3f",
	        &kDefaultAngularDiameterDegrees))
	{
		lightDesc.angularDiameterRadians = MathUtils::DegreesToRadians((std::max) (0.0f, angularDiameterDegrees));
		changed = true;
	}
	UiUtil::EndDetailsCategory();
	return changed;
}

bool SceneLightInspector::BuildPointLightCategory(const std::string& filterText, PointLightDesc& lightDesc) noexcept
{
	bool changed = false;
	if (!UiUtil::MatchesDetailsFilter(filterText, "Point Light", "range radius attenuation point"))
	{
		return false;
	}

	if (!UiUtil::BeginDetailsCategory("Point Light"))
	{
		return false;
	}

	float range = lightDesc.range;
	constexpr float kDefaultRange = 0.0f;
	if (UiUtil::EditDetailsFloat("Range", range, 0.05f, 0.0f, kRangeSliderMax, "%.3f", &kDefaultRange))
	{
		lightDesc.range = (std::max) (0.0f, range);
		changed = true;
	}

	float sourceRadius = lightDesc.sourceRadius;
	constexpr float kDefaultSourceRadius = 0.05f;
	if (UiUtil::EditDetailsFloat(
	        "Source Radius",
	        sourceRadius,
	        0.01f,
	        0.0f,
	        SceneLightInspectorConstants::kSourceRadiusSliderMax,
	        "%.3f",
	        &kDefaultSourceRadius))
	{
		lightDesc.sourceRadius = (std::max) (0.0f, sourceRadius);
		changed = true;
	}

	UiUtil::EndDetailsCategory();
	return changed;
}

bool SceneLightInspector::BuildSpotLightCategory(const std::string& filterText, SpotLightDesc& lightDesc) noexcept
{
	bool changed = false;
	if (!UiUtil::MatchesDetailsFilter(filterText, "Spot Light", "direction range cone angle spot"))
	{
		return false;
	}

	if (!UiUtil::BeginDetailsCategory("Spot Light"))
	{
		return false;
	}

	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	const float defaultDirection[3] = {0.0f, -1.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Direction", directionValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultDirection))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
		changed = true;
	}

	float range = lightDesc.range;
	constexpr float kDefaultRange = 0.0f;
	if (UiUtil::EditDetailsFloat("Range", range, 0.05f, 0.0f, kRangeSliderMax, "%.3f", &kDefaultRange))
	{
		lightDesc.range = (std::max) (0.0f, range);
		changed = true;
	}

	float sourceRadius = lightDesc.sourceRadius;
	constexpr float kDefaultSourceRadius = 0.05f;
	if (UiUtil::EditDetailsFloat(
	        "Source Radius",
	        sourceRadius,
	        0.01f,
	        0.0f,
	        SceneLightInspectorConstants::kSourceRadiusSliderMax,
	        "%.3f",
	        &kDefaultSourceRadius))
	{
		lightDesc.sourceRadius = (std::max) (0.0f, sourceRadius);
		changed = true;
	}

	float innerConeDegrees = MathUtils::RadiansToDegrees(lightDesc.innerConeAngleRadians);
	constexpr float kDefaultInnerConeDegrees = 0.0f;
	if (UiUtil::EditDetailsFloat("Inner Cone", innerConeDegrees, 0.1f, 0.0f, 180.0f, "%.2f", &kDefaultInnerConeDegrees))
	{
		lightDesc.innerConeAngleRadians = MathUtils::DegreesToRadians((std::max) (0.0f, innerConeDegrees));
		changed = true;
	}

	float outerConeDegrees = MathUtils::RadiansToDegrees(lightDesc.outerConeAngleRadians);
	constexpr float kDefaultOuterConeDegrees = 45.0f;
	if (UiUtil::EditDetailsFloat("Outer Cone", outerConeDegrees, 0.1f, 0.0f, 180.0f, "%.2f", &kDefaultOuterConeDegrees))
	{
		lightDesc.outerConeAngleRadians = MathUtils::DegreesToRadians((std::max) (0.0f, outerConeDegrees));
		changed = true;
	}

	UiUtil::EndDetailsCategory();
	return changed;
}

bool SceneLightInspector::BuildRectLightCategory(const std::string& filterText, RectLightDesc& lightDesc) noexcept
{
	bool changed = false;
	if (!UiUtil::MatchesDetailsFilter(filterText, "Rect Light", "direction tangent width height area rectangle quad shadow"))
	{
		return false;
	}

	if (!UiUtil::BeginDetailsCategory("Rect Light"))
	{
		return false;
	}

	float directionValues[3] = {lightDesc.direction.x, lightDesc.direction.y, lightDesc.direction.z};
	const float defaultDirection[3] = {0.0f, -1.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Direction", directionValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultDirection))
	{
		lightDesc.direction = {directionValues[0], directionValues[1], directionValues[2]};
		changed = true;
	}

	float tangentValues[3] = {lightDesc.tangent.x, lightDesc.tangent.y, lightDesc.tangent.z};
	const float defaultTangent[3] = {1.0f, 0.0f, 0.0f};
	if (UiUtil::EditDetailsFloat3("Tangent", tangentValues, 0.01f, kDirectionSliderMin, kDirectionSliderMax, "%.3f", defaultTangent))
	{
		lightDesc.tangent = {tangentValues[0], tangentValues[1], tangentValues[2]};
		changed = true;
	}

	float width = lightDesc.width;
	constexpr float kDefaultSize = 1.0f;
	if (UiUtil::EditDetailsFloat("Width", width, 0.05f, 0.0f, SceneLightInspectorConstants::kAreaLightSizeSliderMax, "%.3f", &kDefaultSize))
	{
		lightDesc.width = (std::max) (0.0f, width);
		changed = true;
	}

	float height = lightDesc.height;
	if (UiUtil::EditDetailsFloat("Height", height, 0.05f, 0.0f, SceneLightInspectorConstants::kAreaLightSizeSliderMax, "%.3f", &kDefaultSize))
	{
		lightDesc.height = (std::max) (0.0f, height);
		changed = true;
	}

	bool castShadow = lightDesc.castShadow;
	constexpr bool kDefaultCastShadow = true;
	if (UiUtil::EditDetailsCheckbox("Cast Shadow", castShadow, &kDefaultCastShadow))
	{
		lightDesc.castShadow = castShadow;
		changed = true;
	}

	UiUtil::EndDetailsCategory();
	return changed;
}
