#pragma once

#include <string>

class GameScene;
class CameraComponent;
class SceneCamera;

class SceneCameraInspector final
{
  public:
	static void Build(GameScene& gameScene, const std::string& filterText) noexcept;

  private:
	static void BuildTransformCategory(const std::string& filterText, CameraComponent& cameraComponent) noexcept;
	static void BuildCameraCategory(const std::string& filterText, CameraComponent& cameraComponent) noexcept;
	static void BuildMovementCategory(const std::string& filterText, SceneCamera& sceneCamera) noexcept;
	static void BuildAdvancedParametersCategory(const std::string& filterText, CameraComponent& cameraComponent) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kScaleSliderMin = 0.001f;
	static constexpr float kScaleSliderMax = 100.0f;
};
