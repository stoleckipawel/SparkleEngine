#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"

#include <array>
#include <string>
#include <vector>

class ConsoleSession;
struct ImGuiInputTextCallbackData;
struct ImVec4;

class ConsolePanel final
{
  public:
	explicit ConsolePanel(ConsoleSession& session) noexcept;

	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void RequestFocus() noexcept;
	void BuildUI(bool disableInteraction);

  private:
	static int HandleInputTextCallback(ImGuiInputTextCallbackData* data);
	static ImVec4 GetSeverityColor(ConsoleCommandSeverity severity) noexcept;
	static std::size_t FindCompletionTokenStart(const std::string& input) noexcept;
	static void ReplaceInputText(ImGuiInputTextCallbackData& data, const std::string& text);

	int HandleHistoryCallback(ImGuiInputTextCallbackData& data);
	int HandleCompletionCallback(ImGuiInputTextCallbackData& data);
	void SubmitInput();
	void DrawToolbar(bool disableInteraction);
	void DrawOutputRecords();
	void DrawInputLine(bool disableInteraction);
	void DrawAutocompletePreview();
	std::vector<std::string> GetCurrentCompletions() const;
	std::string BuildCompletionList(const std::vector<std::string>& completions) const;

	ConsoleSession* m_session = nullptr;
	std::array<char, 512> m_inputBuffer{};
	std::array<char, 128> m_filterBuffer{};
	std::size_t m_seenOutputCount = 0;
	bool m_isOpen = true;
	bool m_scrollToBottom = true;
	bool m_focusInput = true;
};
