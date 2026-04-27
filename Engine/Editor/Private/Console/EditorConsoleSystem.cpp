#include "PCH.h"

#include "Console/EditorConsoleSystem.h"

#include "Core/Public/Console/ConsoleBuiltinCommands.h"
#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"
#include "Core/Public/Console/ConsoleSession.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Panels/OutputLogPanel.h"
#include "Style/SparkleUiPalette.h"

#include <imgui.h>

namespace
{
	constexpr std::uint32_t kWindowsKeyDownMessage = 0x0100;
	constexpr std::uintptr_t kTildeKey = 0xC0;

	ConsoleCommandSeverity ToConsoleSeverity(spdlog::level::level_enum level) noexcept
	{
		if (level >= spdlog::level::err)
		{
			return ConsoleCommandSeverity::Error;
		}
		if (level >= spdlog::level::warn)
		{
			return ConsoleCommandSeverity::Warning;
		}
		return ConsoleCommandSeverity::Info;
	}

	const char* ToSeverityLabel(spdlog::level::level_enum level) noexcept
	{
		if (level >= spdlog::level::err)
		{
			return "error";
		}
		if (level >= spdlog::level::warn)
		{
			return "warning";
		}
		return "info";
	}
}

EditorConsoleSystem::EditorConsoleSystem()
{
	m_commandRegistry = std::make_unique<ConsoleCommandRegistry>();
	ConsoleBuiltinCommands::Register(*m_commandRegistry);
	m_session = std::make_unique<ConsoleSession>(*m_commandRegistry, ConsoleCommandContext{.Scope = ConsoleCommandScope::Editor});
	m_outputLogPanel = std::make_unique<OutputLogPanel>(*m_session);
	m_outputLogPanel->AddLine(ConsoleCommandSeverity::Info, "Output Log panel initialized. Listening to engine log records.");
	SubscribeToLogStream();
}

EditorConsoleSystem::~EditorConsoleSystem() noexcept
{
	if (m_logRecordHandlerId != 0)
	{
		Logging::RemoveRecordHandler(m_logRecordHandlerId);
		m_logRecordHandlerId = 0;
	}
}

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
	OpenOutputLog();
}

void EditorConsoleSystem::OpenOutputLog() noexcept
{
	if (m_outputLogPanel)
	{
		m_outputLogPanel->SetOpen(true);
		m_outputLogPanel->RequestFocus();
	}
}

bool EditorConsoleSystem::HandleShortcut(std::uint32_t message, std::uintptr_t key, bool wantsTextInput) noexcept
{
	if (message == kWindowsKeyDownMessage && key == kTildeKey && !wantsTextInput)
	{
		RequestConsoleFocus();
		return true;
	}
	return false;
}

void EditorConsoleSystem::BuildUI(bool disableInteraction)
{
	if (m_outputLogPanel)
	{
		m_outputLogPanel->BuildUI(disableInteraction);
	}
}

void EditorConsoleSystem::BuildDockedUI(float left, float top, float width, float height, bool disableInteraction)
{
	if (width <= 0.0f || height <= 0.0f)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(left, top), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, SparkleUiPalette::WindowBackground());
	ImGui::PushStyleColor(ImGuiCol_Tab, SparkleUiPalette::TabBackground());
	ImGui::PushStyleColor(ImGuiCol_TabHovered, SparkleUiPalette::TabBackgroundHovered());
	ImGui::PushStyleColor(ImGuiCol_TabActive, SparkleUiPalette::TabBackgroundActive());
	const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoResize |
	    ImGuiWindowFlags_NoCollapse |
	    ImGuiWindowFlags_NoTitleBar |
	    ImGuiWindowFlags_NoSavedSettings;
	if (!ImGui::Begin("Viewport Console Dock", nullptr, windowFlags))
	{
		ImGui::End();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(2);
		return;
	}

	if (m_outputLogPanel)
	{
		m_outputLogPanel->BuildContent(disableInteraction);
	}

	ImGui::End();
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(2);
}

void EditorConsoleSystem::SubscribeToLogStream()
{
	if (m_logRecordHandlerId != 0 || m_outputLogPanel == nullptr)
	{
		return;
	}

	m_logRecordHandlerId = Logging::AddRecordHandler(
	    [this](Logging::LogRecord record)
	    {
		    if (m_outputLogPanel == nullptr)
		    {
			    return;
		    }

		    const char* severityLabel = ToSeverityLabel(record.Level);
		    std::string text;
		    text.reserve(record.LoggerName.size() + record.Message.size() + 16);
		    text += '[';
		    text += record.LoggerName;
		    text += "] [";
		    text += severityLabel;
		    text += "] ";
		    text += record.Message;
		    m_outputLogPanel->AddRecord(ConsoleOutputRecord{.Severity = ToConsoleSeverity(record.Level), .Text = std::move(text)});
	    });
}
