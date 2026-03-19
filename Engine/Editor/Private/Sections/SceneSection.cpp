#include "PCH.h"
#include "Sections/SceneSection.h"

#include "Level/Level.h"
#include "Runtime/Level/LevelManager.h"
#include "Util/UiUtil.h"

#include <algorithm>
#include <cstdio>

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

	const Level* activeLevel = m_levelManager->GetActiveLevel();
	const std::string activeLevelName = activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string();
	const char* previewValue = activeLevelName.empty() ? "<None>" : activeLevelName.c_str();
	char levelCountText[16] = {};
	std::snprintf(levelCountText, sizeof(levelCountText), "%zu", levelNames.size());

	UiUtil::DrawKeyValueRow("Active", previewValue);
	UiUtil::DrawKeyValueRow("State", m_levelManager->IsLevelChangeInProgress() ? "Switching" : "Ready");
	UiUtil::DrawKeyValueRow("Loaded", levelCountText);
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	ImGui::TextDisabled("Load Level");
	ImGui::SetNextItemWidth(-1.0f);

	if (ImGui::BeginCombo("##Level", previewValue))
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