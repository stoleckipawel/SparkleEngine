#pragma once

#include <memory>
#include <string>

class EditorSceneModel;
class EditorTransactionManager;
struct SceneObjectSelection;

class SceneInspectorPanel final
{
  public:
	SceneInspectorPanel(SceneObjectSelection& selection, EditorTransactionManager& transactions, float widthPixels = 560.0f) noexcept;
	void SetWidth(float widthPixels) noexcept;
	float GetWidth() const noexcept { return m_widthPixels; }
	void SetTopInset(float topInsetPixels) noexcept;
	void SetModel(std::shared_ptr<const EditorSceneModel> model) noexcept { m_model = std::move(model); }
	void BuildUI(bool disableInteraction = false);

  private:
	std::string BuildSelectionTitle() const;
	const char* BuildSelectionSubtitle() const noexcept;
	void BuildSelectionHeader() noexcept;
	void BuildDetailsToolbar() noexcept;
	void BuildSelectionInspector() noexcept;

	std::shared_ptr<const EditorSceneModel> m_model;
	EditorTransactionManager* m_transactions = nullptr;
	SceneObjectSelection* m_selection = nullptr;
	float m_widthPixels = 560.0f;
	float m_topInsetPixels = 0.0f;
	std::string m_filterText;
};
