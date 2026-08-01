#pragma once

#include "Diagnostics/RhiDiagnostics.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

struct RhiTimestampQueryLocation final
{
	std::uint32_t PoolIndex = 0;
	std::uint32_t QueryIndex = 0;
};

class RhiTimestampQueryAllocator final
{
  public:
	RhiTimestampQueryAllocator(std::uint32_t poolCount, std::uint32_t queriesPerPool);

	RhiTimestampQueryHandle Allocate(std::uint32_t poolIndex);
	void Release(RhiTimestampQueryHandle query) noexcept;
	RhiTimestampQueryLocation Resolve(RhiTimestampQueryHandle query) const noexcept;

  private:
	std::vector<std::vector<std::uint32_t>> m_freeQueryIndices;
	std::unordered_map<std::uint32_t, RhiTimestampQueryLocation> m_queryLocations;
	std::uint32_t m_nextHandleValue = 1;
	mutable std::mutex m_mutex;
};
