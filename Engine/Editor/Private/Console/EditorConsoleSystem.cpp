#include "PCH.h"

#include "Console/EditorConsoleSystem.h"

#include "Core/Public/Console/ConsoleBuiltinCommands.h"
#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"
#include "Core/Public/Console/ConsoleSession.h"
#include "Panels/EditorConsolePanel.h"
#include "Style/SparkleUiPalette.h"

#include <imgui.h>

#include <algorithm>

class EditorConsoleSystemConstants final
{
  public:
	static constexpr std::uint32_t kWindowsKeyDownMessage = 0x0100;
	static constexpr std::uintptr_t kTildeKey = 0xC0;
	static constexpr float kMinimumDockHeight = 160.0f;
	static constexpr float kMinimumViewportHeight = 64.0f;
};

EditorConsoleSystem::EditorConsoleSystem()
{
	m_commandRegistry = std::make_unique<ConsoleCommandRegistry>();
	ConsoleBuiltinCommands::Register(*m_commandRegistry);
	m_session = std::make_unique<ConsoleSession>(*m_commandRegistry, ConsoleCommandContext{.Scope = ConsoleCommandScope::Editor});
	m_consolePanel = std::make_unique<EditorConsolePanel>(*m_session);
}

EditorConsoleSystem::~EditorConsoleSystem() noexcept = default;

ConsoleCommandRegistry& EditorConsoleSystem::GetCommandRegistry() noexcept
{
	return *m_commandRegistry;
}

void EditorConsoleSystem::SubmitLine(std::string_view line)
{
	if (m_session)
	{
		m_session->SubmitLine(line);
	}
}

void EditorConsoleSystem::AppendOutput(ConsoleOutputRecord record)
{
	if (m_session)
	{
		m_session->Append(std::move(record));
	}
}

void EditorConsoleSystem::RequestConsoleFocus() noexcept
{
	OpenConsole();
}

void EditorConsoleSystem::OpenConsole() noexcept
{
	if (m_consolePanel)
	{
		m_consolePanel->SetOpen(true);
		m_consolePanel->RequestFocus();
	}
}

bool EditorConsoleSystem::HandleShortcut(std::uint32_t message, std::uintptr_t key, bool wantsTextInput) noexcept
{
	if (message == EditorConsoleSystemConstants::kWindowsKeyDownMessage && key == EditorConsoleSystemConstants::kTildeKey && !wantsTextInput)
	{
		RequestConsoleFocus();
		return true;
	}
	return false;
}

void EditorConsoleSystem::BuildUI(bool disableInteraction)
{
	if (m_consolePanel)
	{
		m_consolePanel->BuildUI(disableInteraction);
	}
}

float EditorConsoleSystem::GetDockHeight(float availableHeight) noexcept
{
	const float maxDockHeight = (std::max) (0.0f, availableHeight - EditorConsoleSystemConstants::kMinimumViewportHeight);
	if (maxDockHeight <= 0.0f)
	{
		m_dockHeight = 0.0f;
		return m_dockHeight;
	}

	const float minDockHeight = (std::min) (EditorConsoleSystemConstants::kMinimumDockHeight, maxDockHeight);
	m_dockHeight = (std::clamp) (m_dockHeight, minDockHeight, maxDockHeight);
	return m_dockHeight;
}

void EditorConsoleSystem::BuildDockedUI(float left, float bottom, float width, float availableHeight, bool disableInteraction)
{
	const float height = GetDockHeight(availableHeight);
	if (width <= 0.0f || height <= 0.0f)
	{
		return;
	}

	const float top = bottom - height;
	const float maxDockHeight = (std::max) (0.0f, availableHeight - EditorConsoleSystemConstants::kMinimumViewportHeight);
	const float minDockHeight = (std::min) (EditorConsoleSystemConstants::kMinimumDockHeight, maxDockHeight);
	ImGui::SetNextWindowPos(ImVec2(left, top), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(ImVec2(width, minDockHeight), ImVec2(width, maxDockHeight));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, SparkleUiPalette::WindowBackground());
	ImGui::PushStyleColor(ImGuiCol_Tab, SparkleUiPalette::TabBackground());
	ImGui::PushStyleColor(ImGuiCol_TabHovered, SparkleUiPalette::TabBackgroundHovered());
	ImGui::PushStyleColor(ImGuiCol_TabActive, SparkleUiPalette::TabBackgroundActive());
	const ImGuiWindowFlags windowFlags =
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
	if (!ImGui::Begin("Viewport Console Dock", nullptr, windowFlags))
	{
		ImGui::End();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(2);
		return;
	}

	if (m_consolePanel)
	{
		m_consolePanel->BuildContent(disableInteraction);
	}
	m_dockHeight = ImGui::GetWindowHeight();
	GetDockHeight(availableHeight);

	ImGui::End();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(2);
}
