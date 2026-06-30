#pragma once

#include <cstddef>
#include <string>

class GameScene;
struct PointLightDesc;
struct RectLightDesc;
struct SceneDirectionalLightDesc;
struct SceneLightDesc;
struct SpotLightDesc;

class SceneLightInspector final
{
  public:
	static void Build(GameScene& gameScene, std::size_t lightIndex, const std::string& filterText) noexcept;

  private:
	static void BuildGenericLight(
	    GameScene& gameScene,
	    std::size_t lightIndex,
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
