#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"

#include <array>
#include <mutex>
#include <string>
#include <vector>

struct ImVec4;

class OutputLogPanel final
{
  public:
	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void AddLine(ConsoleCommandSeverity severity, std::string text);
	void AddRecord(ConsoleOutputRecord record);
	void Clear() noexcept;
	void BuildUI(bool disableInteraction);
	void BuildContent(bool disableInteraction);

  private:
	void DrainPendingRecords();
	static ImVec4 GetSeverityColor(ConsoleCommandSeverity severity) noexcept;

	std::vector<ConsoleOutputRecord> m_records;
	std::vector<ConsoleOutputRecord> m_pendingRecords;
	std::mutex m_recordsMutex;
	std::array<char, 128> m_filterBuffer{};
	bool m_isOpen = true;
	bool m_scrollToBottom = true;
};
