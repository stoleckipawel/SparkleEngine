#pragma once

#include <cstddef>
#include <string>
#include "World/EntityId.h"

class GameScene;
class Mesh;
class SceneMeshView;

class SceneMeshInspector final
{
  public:
	static void Build(GameScene& gameScene, EntityId meshEntity, const std::string& filterText) noexcept;

  private:
	static void BuildTransformCategory(const std::string& filterText, SceneMeshView& mesh) noexcept;
	static void BuildStaticMeshCategory(const std::string& filterText, const Mesh& mesh, SceneMeshView& instance) noexcept;
	static void BuildStaticMeshAdvancedCategory(const std::string& filterText, const Mesh& mesh) noexcept;
	static void BuildAdvancedParametersCategory(const std::string& filterText, SceneMeshView& mesh) noexcept;
	static void BuildMaterialsCategory(const std::string& filterText, const SceneMeshView& mesh) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kScaleSliderMin = 0.001f;
	static constexpr float kScaleSliderMax = 100.0f;
};
