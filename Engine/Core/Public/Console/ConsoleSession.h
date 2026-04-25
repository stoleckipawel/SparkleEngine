#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleHistoryBuffer.h"
#include "Core/Public/Console/ConsoleOutput.h"
#include "Core/Public/CoreAPI.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class SPARKLE_CORE_API ConsoleSession final : public ConsoleOutputSink
{
  public:
	ConsoleSession(ConsoleCommandRegistry& commandRegistry, ConsoleCommandContext context = {});

	void SubmitLine(std::string_view line);
	std::vector<std::string> CompleteLine(std::string_view line) const;

	void Append(ConsoleOutputRecord record) override;
	void AddOutput(ConsoleCommandSeverity severity, std::string text);
	void ClearOutput() noexcept;

	std::optional<std::string> NavigateHistoryPrevious(std::string_view currentLine);
	std::optional<std::string> NavigateHistoryNext();
	void ResetHistoryNavigation() noexcept;

	const std::vector<ConsoleOutputRecord>& GetOutputRecords() const noexcept { return m_outputRecords; }
	const ConsoleHistoryBuffer& GetHistory() const noexcept { return m_history; }
	ConsoleHistoryBuffer& GetHistory() noexcept { return m_history; }
	const ConsoleCommandContext& GetContext() const noexcept { return m_context; }
	void SetContext(ConsoleCommandContext context) noexcept { m_context = context; }

  private:
	static constexpr std::size_t kMaxOutputRecords = 512;

	ConsoleCommandRegistry* m_commandRegistry = nullptr;
	ConsoleCommandContext m_context;
	ConsoleHistoryBuffer m_history;
	std::vector<ConsoleOutputRecord> m_outputRecords;
};
