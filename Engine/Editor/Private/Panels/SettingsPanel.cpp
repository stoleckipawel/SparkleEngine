#include "PCH.h"

#include "Panels/SettingsPanel.h"

#include "Panels/RenderingSettingsPanel.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <string>
#include <utility>

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

void SettingsPanel::SetRestartHandler(std::function<void()> restartHandler)
{
	m_restartHandler = std::move(restartHandler);
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

	ImGui::SetNextWindowSize(ImVec2(1120.0f, 780.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Settings, "Settings") + "##EditorSettings";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	const bool showRestartBar = HasPendingRestart();
	const float restartBarHeight = showRestartBar ? 38.0f : 0.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 12.0f));
	ImGui::BeginChild("##SettingsContent", ImVec2(0.0f, -restartBarHeight));
	DrawToolbar();
	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	if (m_renderingPanel != nullptr)
	{
		m_renderingPanel->BuildUI(disableInteraction, m_filterText.c_str());
	}
	ImGui::EndChild();

	if (showRestartBar)
	{
		ImGui::Separator();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 6.0f));
		ImGui::BeginChild("##SettingsFooter", ImVec2(0.0f, restartBarHeight - 8.0f));
		ImGui::BeginDisabled(!m_restartHandler);
		const float buttonWidth = 96.0f;
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
		ImGui::SetCursorPosX((std::max)(0.0f, ImGui::GetContentRegionAvail().x - buttonWidth));
		if (ImGui::Button("Restart", ImVec2(buttonWidth, 0.0f)) && m_restartHandler)
		{
			m_restartHandler();
		}
		ImGui::EndDisabled();
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	ImGui::PopStyleVar();

	ImGui::End();
}

void SettingsPanel::DrawToolbar()
{
	std::array<char, 128> filterBuffer{};
	const std::size_t copyLength = (std::min)(m_filterText.size(), filterBuffer.size() - 1);
	if (copyLength > 0)
	{
		m_filterText.copy(filterBuffer.data(), copyLength);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
	ImGui::SetNextItemWidth(-FLT_MIN);
	if (ImGui::InputTextWithHint(
	        "##SettingsSearch",
	        UiUtil::MakeIconLabel(UiUtil::EditorIcon::Search, "Search").c_str(),
	        filterBuffer.data(),
	        filterBuffer.size()))
	{
		m_filterText = filterBuffer.data();
	}
	ImGui::PopStyleVar();
}

bool SettingsPanel::HasPendingRestart() const noexcept
{
	return m_renderingPanel != nullptr && m_renderingPanel->HasPendingRestart();
}
