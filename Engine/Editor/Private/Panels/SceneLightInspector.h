#pragma once

#include <cstddef>
#include <string>
#include "World/EntityId.h"

class GameWorld;
struct PointLightDesc;
struct RectLightDesc;
struct SceneDirectionalLightDesc;
struct SceneLightDesc;
struct SpotLightDesc;

class SceneLightInspector final
{
  public:
	static void Build(GameWorld& gameWorld, EntityId lightEntity, const std::string& filterText) noexcept;

  private:
	static void BuildGenericLight(
	    GameWorld& gameWorld,
	    EntityId lightEntity,
	    const SceneLightDesc& sceneLight,
	    const std::string& filterText) noexcept;
	static void BuildLightCommonCategory(const std::string& filterText, SceneLightDesc& lightDesc) noexcept;
	static void BuildDirectionalLightTransformCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept;
	static void BuildDirectionalLightCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept;
	static void BuildPointLightCategory(const std::string& filterText, PointLightDesc& lightDesc) noexcept;
	static void BuildSpotLightCategory(const std::string& filterText, SpotLightDesc& lightDesc) noexcept;
	static void BuildRectLightCategory(const std::string& filterText, RectLightDesc& lightDesc) noexcept;

	static constexpr float kDirectionSliderMin = -1.0f;
	static constexpr float kDirectionSliderMax = 1.0f;
	static constexpr float kRangeSliderMax = 500.0f;
};
