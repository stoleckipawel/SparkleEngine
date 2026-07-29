#pragma once

#include <cstddef>
#include <string>
#include "World/EntityId.h"

class EditorTransactionHistory;
struct PointLightDesc;
struct RectLightDesc;
struct SceneDirectionalLightDesc;
struct SceneLightDesc;
struct SpotLightDesc;

class SceneLightInspector final
{
  public:
	static void Build(const SceneLightDesc&, EntityId, EditorTransactionHistory&, std::uint64_t, const std::string&) noexcept;

  private:
	static void BuildGenericLight(
	    EditorTransactionHistory& transactionHistory,
	    std::uint64_t worldGeneration,
	    EntityId lightEntity,
	    const SceneLightDesc& sceneLight,
	    const std::string& filterText) noexcept;
	static bool BuildLightCommonCategory(const std::string& filterText, SceneLightDesc& lightDesc) noexcept;
	static bool BuildDirectionalLightTransformCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept;
	static bool BuildDirectionalLightCategory(const std::string& filterText, SceneDirectionalLightDesc& lightDesc) noexcept;
	static bool BuildPointLightCategory(const std::string& filterText, PointLightDesc& lightDesc) noexcept;
	static bool BuildSpotLightCategory(const std::string& filterText, SpotLightDesc& lightDesc) noexcept;
	static bool BuildRectLightCategory(const std::string& filterText, RectLightDesc& lightDesc) noexcept;

	static constexpr float kDirectionSliderMin = -1.0f;
	static constexpr float kDirectionSliderMax = 1.0f;
	static constexpr float kRangeSliderMax = 500.0f;
};
