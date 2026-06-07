#pragma once

#include <cstddef>
#include <string>

class GameScene;
class Mesh;
class MeshComponent;

class SceneMeshInspector final
{
  public:
	static void Build(GameScene& gameScene, std::size_t meshIndex, const std::string& filterText) noexcept;

  private:
	static void BuildTransformCategory(const std::string& filterText, MeshComponent& meshComponent) noexcept;
	static void BuildStaticMeshCategory(const std::string& filterText, const Mesh& mesh, MeshComponent& meshComponent) noexcept;
	static void BuildStaticMeshAdvancedCategory(const std::string& filterText, const Mesh& mesh) noexcept;
	static void BuildAdvancedParametersCategory(const std::string& filterText, MeshComponent& meshComponent) noexcept;
	static void BuildMaterialsCategory(const std::string& filterText, const MeshComponent& meshComponent) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kScaleSliderMin = 0.001f;
	static constexpr float kScaleSliderMax = 100.0f;
};
