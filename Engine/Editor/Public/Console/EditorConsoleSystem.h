#pragma once

#include "EditorAPI.h"

#include <cstdint>
#include <memory>
#include <string_view>

class ConsoleCommandRegistry;
class ConsolePanel;
class ConsoleSession;
class OutputLogPanel;
struct ConsoleOutputRecord;

class SPARKLE_EDITOR_API EditorConsoleSystem final
{
  public:
	EditorConsoleSystem();
	~EditorConsoleSystem() noexcept;

	EditorConsoleSystem(const EditorConsoleSystem&) = delete;
	EditorConsoleSystem& operator=(const EditorConsoleSystem&) = delete;
	EditorConsoleSystem(EditorConsoleSystem&&) = delete;
	EditorConsoleSystem& operator=(EditorConsoleSystem&&) = delete;

	ConsoleCommandRegistry& GetCommandRegistry() noexcept;
	void SubmitLine(std::string_view line);
	void AppendOutput(ConsoleOutputRecord record);
	void RequestConsoleFocus() noexcept;
	void OpenOutputLog() noexcept;
	bool HandleShortcut(std::uint32_t message, std::uintptr_t key, bool wantsTextInput) noexcept;
	void BuildUI(bool disableInteraction);
	void BuildDockedUI(float left, float top, float width, float height, bool disableInteraction);

  private:
	enum class ActiveDockTab : std::uint8_t
	{
		OutputLog,
		Console,
	};

	void SubscribeToLogStream();

	std::unique_ptr<ConsoleCommandRegistry> m_commandRegistry;
	std::unique_ptr<ConsoleSession> m_session;
	std::unique_ptr<ConsolePanel> m_consolePanel;
	std::unique_ptr<OutputLogPanel> m_outputLogPanel;
	std::uint64_t m_logRecordHandlerId = 0;
	ActiveDockTab m_activeDockTab = ActiveDockTab::OutputLog;
};
