#pragma once

#include "GameFramework/Public/World/WorldChange.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace ECS
{
	class WorldChangeJournal final
	{
	public:
		static constexpr std::size_t MaxRetainedBatches = 64;
		static constexpr std::size_t MaxChangesPerBatch = 4096;

		WorldSequence Publish(std::span<const WorldChange> changes);
		WorldChangeBatch ReadAfter(WorldSequence acknowledgedSequence) const;
		WorldSequence GetLatestSequence() const;

	private:
		mutable std::mutex m_mutex;
		std::deque<std::shared_ptr<const WorldChangeBatch::Storage>> m_batches;
		WorldSequence m_latestSequence = 0;
	};
}
