#include "PCH.h"
#include "Sections/SceneSection.h"

#include "Runtime/Level/LevelManager.h"

#include <algorithm>

#include <imgui.h>

SceneSection::SceneSection(LevelManager& levelManager) noexcept : m_levelManager(&levelManager) {}

void SceneSection::BuildUI()
{
	if (m_levelManager == nullptr)
	{
		ImGui::TextDisabled("Scene selector unavailable");
		return;
	}

	std::vector<std::string> levelNames = m_levelManager->GetRegisteredLevelNames();

	std::sort(levelNames.begin(), levelNames.end());

	const std::string activeLevelName(m_levelManager->GetActiveLevelName());
	const char* previewValue = activeLevelName.empty() ? "<None>" : activeLevelName.c_str();

	ImGui::Text("Active Level: %s", previewValue);
	ImGui::Text("Status: %s", m_levelManager->IsLevelChangeInProgress() ? "Changing level" : "Ready");
	ImGui::Text("Registered Levels: %zu", levelNames.size());

	if (ImGui::BeginCombo("Level", previewValue))
	{
		for (const std::string& levelName : levelNames)
		{
			const bool isSelected = levelName == activeLevelName;
			if (ImGui::Selectable(levelName.data(), isSelected) && !isSelected)
			{
				m_levelManager->RequestLevelChange(levelName);
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}
}