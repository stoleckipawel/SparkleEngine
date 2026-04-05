#pragma once

#include <cstddef>

#include <string>

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
	void DrawSelectionEntry(const char* label, const char* typeLabel, const SceneObjectSelection& selection) noexcept;

	GameScene* m_gameScene = nullptr;
	SceneObjectSelection* m_selection = nullptr;
	float m_widthPixels = 320.0f;
	float m_topInsetPixels = 0.0f;
	std::string m_filterText;
};