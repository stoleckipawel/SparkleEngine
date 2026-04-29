#pragma once

#include <cstddef>

#include <string>
#include <vector>

class GameScene;
struct SceneObjectSelection;

class SceneOutlinerPanel final
{
  public:
	SceneOutlinerPanel(GameScene& gameScene, SceneObjectSelection& selection, float widthPixels = 320.0f) noexcept;
	~SceneOutlinerPanel() = default;

	SceneOutlinerPanel(const SceneOutlinerPanel&) = delete;
	SceneOutlinerPanel(SceneOutlinerPanel&&) = delete;
	SceneOutlinerPanel& operator=(const SceneOutlinerPanel&) = delete;
	SceneOutlinerPanel& operator=(SceneOutlinerPanel&&) = delete;

	void SetWidth(float widthPixels) noexcept;
	float GetWidth() const noexcept { return m_widthPixels; }
	void SetTopInset(float topInsetPixels) noexcept;
	void BuildUI(bool disableInteraction = false);

  private:
	static std::string BuildMeshLabel(std::size_t meshIndex);
	void BuildToolbar() noexcept;
	bool IsSelectionValid() const noexcept;
	void EnsureValidSelection() noexcept;
	void BuildCameraSection() noexcept;
	void BuildLightSection() noexcept;
	void BuildMeshSection() noexcept;
	void SyncVisibilityState() noexcept;
	bool IsEntryVisible(const SceneObjectSelection& selection) const noexcept;
	void ToggleEntryVisibility(const SceneObjectSelection& selection) noexcept;
	void DrawSelectionEntry(const char* label, const char* iconText, const char* typeLabel, const SceneObjectSelection& selection) noexcept;

	GameScene* m_gameScene = nullptr;
	SceneObjectSelection* m_selection = nullptr;
	float m_widthPixels = 320.0f;
	float m_topInsetPixels = 0.0f;
	bool m_cameraVisible = true;
	std::vector<bool> m_lightVisibility;
	std::vector<bool> m_meshVisibility;
	std::string m_filterText;
};