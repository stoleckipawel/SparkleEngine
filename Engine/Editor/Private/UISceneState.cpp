#include "PCH.h"

#include "UI.h"

#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Scene/Model/EditorSceneModel.h"
#include "Scene/Model/EditorSceneModelBuilder.h"
#include "Scene/Transactions/EditorTransactionHistory.h"

#include <imgui.h>

void UI::UpdateSceneModel()
{
	if (!m_sceneModelBuilder)
	{
		return;
	}

	const std::uint64_t previousWorldGeneration = m_sceneModel ? m_sceneModel->GetWorldGeneration() : 0;
	m_sceneModel = m_sceneModelBuilder->Update();
	if (!m_sceneModel)
	{
		return;
	}

	if (m_transactionHistory)
	{
		m_transactionHistory->InvalidateForWorldGeneration(m_sceneModel->GetWorldGeneration());
	}
	if (previousWorldGeneration != 0 && previousWorldGeneration != m_sceneModel->GetWorldGeneration())
	{
		m_sceneSelection = SceneObjectSelection::None();
	}
	if (!m_sceneSelection.IsNone() && !m_sceneModel->Contains(m_sceneSelection))
	{
		m_sceneSelection = SceneObjectSelection::None();
	}

	if (m_sceneOutlinerPanel)
	{
		m_sceneOutlinerPanel->SetModel(m_sceneModel);
	}
	if (m_sceneInspectorPanel)
	{
		m_sceneInspectorPanel->SetModel(m_sceneModel);
	}
}

void UI::HandleTransactionShortcuts()
{
	if (!m_sceneModel || !m_transactionHistory || ImGui::GetIO().WantTextInput)
	{
		return;
	}

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z))
	{
		(void) m_transactionHistory->Undo(m_sceneModel->GetWorldGeneration());
	}
	else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y))
	{
		(void) m_transactionHistory->Redo(m_sceneModel->GetWorldGeneration());
	}
}
