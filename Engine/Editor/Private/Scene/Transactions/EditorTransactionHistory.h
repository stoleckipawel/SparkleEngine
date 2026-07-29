#pragma once

#include "World/WorldEditCommand.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class EditorTransactionHistory final
{
  public:
	using SubmitFunction = std::function<WorldEditResult(WorldEditCommand, std::uint64_t)>;

	EditorTransactionHistory() = default;
	explicit EditorTransactionHistory(SubmitFunction submit) : m_submit(std::move(submit)) {}

	WorldEditResult Execute(
	    WorldEditCommand forward,
	    WorldEditCommand inverse,
	    std::uint64_t worldGeneration,
	    std::string coalescingKey = {});
	WorldEditResult Undo(std::uint64_t worldGeneration);
	WorldEditResult Redo(std::uint64_t worldGeneration);
	void InvalidateForWorldGeneration(std::uint64_t worldGeneration) noexcept;
	bool CanUndo() const noexcept { return !m_undo.empty(); }
	bool CanRedo() const noexcept { return !m_redo.empty(); }
	const std::optional<WorldEditResult>& GetLastResult() const noexcept { return m_lastResult; }

  private:
	struct Transaction final
	{
		WorldEditCommand Forward;
		WorldEditCommand Inverse;
		std::string CoalescingKey;
	};
	WorldEditResult Submit(WorldEditCommand command, std::uint64_t worldGeneration);
	SubmitFunction m_submit;
	std::vector<Transaction> m_undo;
	std::vector<Transaction> m_redo;
	std::optional<WorldEditResult> m_lastResult;
	std::uint64_t m_nextRequestId = 1;
	std::uint64_t m_worldGeneration = 0;
};

