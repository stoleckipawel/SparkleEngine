#pragma once

#include "EditorAPI.h"

#include <cstdint>
#include <memory>
#include <string_view>

class ConsoleCommandRegistry;
class ConsoleSession;
class EditorConsolePanel;
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
	void OpenConsole() noexcept;
	bool HandleShortcut(std::uint32_t message, std::uintptr_t key, bool wantsTextInput) noexcept;
	void BuildUI(bool disableInteraction);
	float GetDockHeight(float availableHeight) noexcept;
	void BuildDockedUI(float left, float bottom, float width, float availableHeight, bool disableInteraction);

private:
	std::unique_ptr<ConsoleCommandRegistry> m_commandRegistry;
	std::unique_ptr<ConsoleSession> m_session;
	std::unique_ptr<EditorConsolePanel> m_consolePanel;
	float m_dockHeight = 300.0f;
};
