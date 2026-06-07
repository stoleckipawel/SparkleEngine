#pragma once

#include <cstddef>

#include <string>

class GameScene;
struct SceneObjectSelection;

class SceneInspectorPanel final
{
  public:
	SceneInspectorPanel(GameScene& gameScene, SceneObjectSelection& selection, float widthPixels = 560.0f) noexcept;
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
	void BuildDetailsToolbar() noexcept;
	void BuildSelectionInspector() noexcept;

	GameScene* m_gameScene = nullptr;
	SceneObjectSelection* m_selection = nullptr;
	float m_widthPixels = 560.0f;
	float m_topInsetPixels = 0.0f;
	std::string m_filterText;
};
