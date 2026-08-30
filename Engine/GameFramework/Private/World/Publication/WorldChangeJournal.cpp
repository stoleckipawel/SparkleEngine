#include "PCH.h"
#include "World/Publication/WorldChangeJournal.h"

#include <algorithm>

struct WorldChangeBatch::Storage final
{
	std::vector<WorldChange> Changes;
};

WorldChangeBatch::WorldChangeBatch(
    WorldChangeReadStatus status,
    WorldSequence oldestAvailableSequence,
    WorldSequence latestSequence,
    std::shared_ptr<const Storage> storage,
    std::size_t firstChangeIndex) noexcept :
    m_status(status),
    m_oldestAvailableSequence(oldestAvailableSequence),
    m_latestSequence(latestSequence),
    m_storage(std::move(storage)),
    m_firstChangeIndex(firstChangeIndex)
{
}

std::span<const WorldChange> WorldChangeBatch::GetChanges() const noexcept
{
	return m_storage != nullptr ? std::span<const WorldChange>(m_storage->Changes).subspan(m_firstChangeIndex)
	                            : std::span<const WorldChange>{};
}

namespace ECS
{
	WorldSequence WorldChangeJournal::Publish(std::span<const WorldChange> changes)
	{
		const std::scoped_lock lock(m_mutex);
		if (changes.empty())
		{
			return m_latestSequence;
		}
		auto storage = std::make_shared<WorldChangeBatch::Storage>();
		if (changes.size() > MaxChangesPerBatch)
		{
			storage->Changes.push_back(WorldChange{.Kind = WorldChangeKind::WorldReset, .Data = WorldDataKind::World});
		}
		else
		{
			storage->Changes.assign(changes.begin(), changes.end());
		}
		for (WorldChange& change : storage->Changes)
		{
			change.Sequence = ++m_latestSequence;
		}
		m_batches.push_back(std::move(storage));
		while (m_batches.size() > MaxRetainedBatches)
		{
			m_batches.pop_front();
		}
		return m_latestSequence;
	}

	WorldChangeBatch WorldChangeJournal::ReadAfter(WorldSequence acknowledgedSequence) const
	{
		const std::scoped_lock lock(m_mutex);
		if (m_batches.empty())
		{
			return WorldChangeBatch(WorldChangeReadStatus::UpToDate, m_latestSequence, m_latestSequence, nullptr);
		}
		const WorldSequence oldest = m_batches.front()->Changes.front().Sequence;
		if (oldest > 0 && acknowledgedSequence < oldest - 1)
		{
			return WorldChangeBatch(WorldChangeReadStatus::ResyncRequired, oldest, m_latestSequence, nullptr);
		}
		for (const std::shared_ptr<const WorldChangeBatch::Storage>& batch : m_batches)
		{
			if (batch->Changes.back().Sequence > acknowledgedSequence)
			{
				const auto first = std::upper_bound(
				    batch->Changes.begin(),
				    batch->Changes.end(),
				    acknowledgedSequence,
				    [](WorldSequence sequence, const WorldChange& change) { return sequence < change.Sequence; });
				return WorldChangeBatch(
				    WorldChangeReadStatus::Available,
				    oldest,
				    m_latestSequence,
				    batch,
				    static_cast<std::size_t>(first - batch->Changes.begin()));
			}
		}
		return WorldChangeBatch(WorldChangeReadStatus::UpToDate, oldest, m_latestSequence, nullptr);
	}

	WorldSequence WorldChangeJournal::GetLatestSequence() const
	{
		const std::scoped_lock lock(m_mutex);
		return m_latestSequence;
	}
}
