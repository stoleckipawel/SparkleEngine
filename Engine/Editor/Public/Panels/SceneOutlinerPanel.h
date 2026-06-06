#pragma once

#include <cstddef>

#include <string>

class GameScene;
struct SceneObjectSelection;

enum class SceneOutlinerFilter
{
	All,
	Cameras,
	Lights,
	Meshes
};

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
	void BuildFooter() noexcept;
	bool IsSelectionValid() const noexcept;
	void EnsureValidSelection() noexcept;
	void BuildCameraSection() noexcept;
	void BuildLightSection() noexcept;
	void BuildMeshSection() noexcept;
	bool PassesActiveFilter(const SceneObjectSelection& selection) const noexcept;
	bool MatchesSearch(const char* label, const char* typeLabel) const noexcept;
	std::size_t CountVisibleEntries() const noexcept;
	bool IsEntryVisible(const SceneObjectSelection& selection) const noexcept;
	void ToggleEntryVisibility(const SceneObjectSelection& selection) noexcept;
	void SelectEntry(const SceneObjectSelection& selection) noexcept;
	void DrawSectionRow(const char* id, const char* label, std::size_t count, bool& open) noexcept;
	void DrawSelectionEntry(const char* label, const char* typeLabel, const SceneObjectSelection& selection) noexcept;

	GameScene* m_gameScene = nullptr;
	SceneObjectSelection* m_selection = nullptr;
	float m_widthPixels = 320.0f;
	float m_topInsetPixels = 0.0f;
	SceneOutlinerFilter m_activeFilter = SceneOutlinerFilter::All;
	std::string m_filterText;
};
