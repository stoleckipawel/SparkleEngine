#include "PCH.h"
#include "Scene/Transactions/EditorTransactionManager.h"

#include <utility>

class EditorTransactionManagerConstants final
{
  public:
	static constexpr std::size_t MaximumTransactions = 256;
};

WorldEditResult EditorTransactionManager::Submit(WorldEditCommand command, std::uint64_t worldGeneration)
{
	command.RequestId = m_nextRequestId++;
	if (!m_submit)
		return {command.RequestId, WorldEditResultStatus::Rejected, "The editor command boundary is unavailable."};
	return m_submit(std::move(command), worldGeneration);
}

void EditorTransactionManager::InvalidateForWorldGeneration(std::uint64_t worldGeneration) noexcept
{
	if (m_worldGeneration == worldGeneration) return;
	m_worldGeneration = worldGeneration;
	m_undo.clear();
	m_redo.clear();
	m_lastResult.reset();
}

WorldEditResult EditorTransactionManager::Execute(
    WorldEditCommand forward,
    WorldEditCommand inverse,
    std::uint64_t worldGeneration,
    std::string coalescingKey)
{
	InvalidateForWorldGeneration(worldGeneration);
	WorldEditResult result = Submit(forward, worldGeneration);
	m_lastResult = result;
	if (!result.IsAccepted()) return result;

	if (!coalescingKey.empty() && !m_undo.empty() && m_undo.back().CoalescingKey == coalescingKey)
	{
		m_undo.back().Forward = std::move(forward);
	}
	else
	{
		if (m_undo.size() == EditorTransactionManagerConstants::MaximumTransactions) m_undo.erase(m_undo.begin());
		m_undo.push_back({std::move(forward), std::move(inverse), std::move(coalescingKey)});
	}
	m_redo.clear();
	return result;
}

WorldEditResult EditorTransactionManager::Undo(std::uint64_t worldGeneration)
{
	InvalidateForWorldGeneration(worldGeneration);
	if (m_undo.empty())
		return {0, WorldEditResultStatus::Rejected, "There is no editor transaction to undo."};
	Transaction transaction = m_undo.back();
	WorldEditResult result = Submit(transaction.Inverse, worldGeneration);
	m_lastResult = result;
	if (result.IsAccepted())
	{
		m_undo.pop_back();
		m_redo.push_back(std::move(transaction));
	}
	return result;
}

WorldEditResult EditorTransactionManager::Redo(std::uint64_t worldGeneration)
{
	InvalidateForWorldGeneration(worldGeneration);
	if (m_redo.empty())
		return {0, WorldEditResultStatus::Rejected, "There is no editor transaction to redo."};
	Transaction transaction = m_redo.back();
	WorldEditResult result = Submit(transaction.Forward, worldGeneration);
	m_lastResult = result;
	if (result.IsAccepted())
	{
		m_redo.pop_back();
		m_undo.push_back(std::move(transaction));
	}
	return result;
}
