#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"

#include <array>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

class ConsoleSession;
struct ImGuiInputTextCallbackData;

class OutputLogPanel final
{
  public:
	explicit OutputLogPanel(ConsoleSession& session) noexcept;

	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void RequestFocus() noexcept;
	void AddLine(ConsoleCommandSeverity severity, std::string text);
	void AddRecord(ConsoleOutputRecord record);
	void Clear() noexcept;
	void BuildUI(bool disableInteraction);
	void BuildContent(bool disableInteraction);

  private:
	static int HandleInputTextCallback(ImGuiInputTextCallbackData* data);
	static std::size_t FindCompletionTokenStart(const std::string& input) noexcept;
	static void ReplaceInputText(ImGuiInputTextCallbackData& data, const std::string& text);

	int HandleHistoryCallback(ImGuiInputTextCallbackData& data);
	int HandleCompletionCallback(ImGuiInputTextCallbackData& data);
	void SubmitInput();
	void DrawToolbar(bool disableInteraction);
	void DrawScrollback();
	void DrawInputLine(bool disableInteraction);
	void DrawAutocompletePreview();
	void BuildOutputTextBuffer(std::string_view filter);
	std::vector<std::string> GetCurrentCompletions() const;
	std::string BuildCompletionList(const std::vector<std::string>& completions) const;
	void DrainPendingRecords();

	ConsoleSession* m_session = nullptr;
	std::vector<ConsoleOutputRecord> m_records;
	std::vector<ConsoleOutputRecord> m_pendingRecords;
	std::string m_outputTextBuffer;
	std::mutex m_recordsMutex;
	std::array<char, 512> m_inputBuffer{};
	std::array<char, 128> m_filterBuffer{};
	std::size_t m_seenConsoleOutputCount = 0;
	bool m_isOpen = true;
	bool m_scrollToBottom = true;
	bool m_focusInput = true;
};
