#include "PCH.h"

#include "Console/EditorConsoleSystem.h"

#include "Core/Public/Console/ConsoleBuiltinCommands.h"
#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"
#include "Core/Public/Console/ConsoleSession.h"
#include "Core/Public/Diagnostics/Logger.h"
#include "Panels/ConsolePanel.h"
#include "Panels/OutputLogPanel.h"

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
}

EditorConsoleSystem::EditorConsoleSystem()
{
	m_commandRegistry = std::make_unique<ConsoleCommandRegistry>();
	ConsoleBuiltinCommands::Register(*m_commandRegistry);
	m_session = std::make_unique<ConsoleSession>(*m_commandRegistry, ConsoleCommandContext{.Scope = ConsoleCommandScope::Editor});
	m_consolePanel = std::make_unique<ConsolePanel>(*m_session);
	m_outputLogPanel = std::make_unique<OutputLogPanel>();
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
	if (m_consolePanel)
	{
		m_consolePanel->RequestFocus();
	}
}

void EditorConsoleSystem::OpenOutputLog() noexcept
{
	if (m_outputLogPanel)
	{
		m_outputLogPanel->SetOpen(true);
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
	if (m_consolePanel)
	{
		m_consolePanel->BuildUI(disableInteraction);
	}
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

		    std::string text;
		    text.reserve(record.LoggerName.size() + record.Message.size() + 4);
		    text += '[';
		    text += record.LoggerName;
		    text += "] ";
		    text += record.Message;
		    m_outputLogPanel->AddRecord(ConsoleOutputRecord{.Severity = ToConsoleSeverity(record.Level), .Text = std::move(text)});
	    });
}
