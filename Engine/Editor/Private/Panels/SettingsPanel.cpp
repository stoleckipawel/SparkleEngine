#include "PCH.h"

#include "Panels/SettingsPanel.h"

#include "Panels/RenderingSettingsPanel.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <string>

SettingsPanel::SettingsPanel() = default;
SettingsPanel::~SettingsPanel() = default;

void SettingsPanel::SetOpen(bool open) noexcept
{
	if (open && !m_isOpen)
	{
		m_refreshFromRuntimeOnNextOpen = true;
	}
	m_isOpen = open;
}

void SettingsPanel::SetRenderingSettings(EngineRenderingSettingsSection* renderingSettings) noexcept
{
	m_renderingSettings = renderingSettings;
	if (!m_renderingPanel)
	{
		m_renderingPanel = std::make_unique<RenderingSettingsPanel>();
	}
	m_renderingPanel->SetSettings(renderingSettings);
}

void SettingsPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen || m_renderingSettings == nullptr)
	{
		return;
	}

	if (m_refreshFromRuntimeOnNextOpen)
	{
		if (m_renderingPanel != nullptr)
		{
			m_renderingPanel->RefreshFromRuntimeState();
		}
		m_refreshFromRuntimeOnNextOpen = false;
	}

	ImGui::SetNextWindowSize(ImVec2(1040.0f, 760.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Settings, "Settings") + "##EditorSettings";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawNavigation();
	ImGui::SameLine();

	ImGui::BeginChild("##SettingsContent", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
	switch (m_activeSection)
	{
		case Section::Rendering:
		default:
			if (m_renderingPanel != nullptr)
			{
				m_renderingPanel->BuildUI(disableInteraction);
			}
			break;
	}
	ImGui::EndChild();

	ImGui::End();
}

void SettingsPanel::DrawNavigation()
{
	ImGui::BeginChild("##SettingsNavigation", ImVec2(210.0f, 0.0f), ImGuiChildFlags_Borders);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));

	const bool renderingSelected = m_activeSection == Section::Rendering;
	if (ImGui::Selectable(UiUtil::MakeIconLabel(UiUtil::EditorIcon::Material, "Rendering").c_str(), renderingSelected, 0, ImVec2(-1.0f, 0.0f)))
	{
		m_activeSection = Section::Rendering;
	}

	ImGui::PopStyleVar(2);
	ImGui::EndChild();
}
