#pragma once

#include <cstddef>

#include <string>

#include <DirectXMath.h>

class GameScene;
class Mesh;
class MeshComponent;
struct SceneObjectSelection;

class SceneInspectorPanel final
{
  public:
	SceneInspectorPanel(
	    GameScene& gameScene,
	    SceneObjectSelection& selection,
	    float widthPixels = 560.0f) noexcept;
	~SceneInspectorPanel() = default;

	SceneInspectorPanel(const SceneInspectorPanel&) = delete;
	SceneInspectorPanel(SceneInspectorPanel&&) = delete;
	SceneInspectorPanel& operator=(const SceneInspectorPanel&) = delete;
	SceneInspectorPanel& operator=(SceneInspectorPanel&&) = delete;

	void SetWidth(float widthPixels) noexcept;
	float GetWidth() const noexcept { return m_widthPixels; }
	void SetTopInset(float topInsetPixels) noexcept;
	void BuildUI(bool disableInteraction = false);

  private:
	std::string BuildSelectionTitle() const;
	const char* BuildSelectionSubtitle() const noexcept;
	void BuildSelectionHeader() noexcept;
	void BuildSelectionInspector() noexcept;
	static void ClampCameraUiValues(float& pitchDegrees, float& fovYDegrees, float& moveSpeed) noexcept;
	static void ClampLightingUiValues(DirectX::XMFLOAT3& color, float& intensity) noexcept;
	void BuildEmptyState() noexcept;
	void BuildCameraInspector() noexcept;
	void BuildDirectionalLightInspector(std::size_t lightIndex) noexcept;
	void BuildMeshInspector(std::size_t meshIndex) noexcept;
	void BuildEditableMeshTransform(MeshComponent& meshComponent) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kYawSliderMin = -360.0f;
	static constexpr float kYawSliderMax = 360.0f;
	static constexpr float kDirectionSliderMin = -1.0f;
	static constexpr float kDirectionSliderMax = 1.0f;
	static constexpr float kIntensitySliderMin = 0.0f;
	static constexpr float kIntensitySliderMax = 20.0f;
	static constexpr float kScaleSliderMin = 0.001f;
	static constexpr float kScaleSliderMax = 100.0f;

	GameScene* m_gameScene = nullptr;
	SceneObjectSelection* m_selection = nullptr;
	float m_widthPixels = 560.0f;
	float m_topInsetPixels = 0.0f;
};