#include "PCH.h"

#include "Diagnostics/RhiTimestampQueryAllocator.h"

#include <limits>

static const auto g_rhiTimestampQueryAllocatorLogger = Logging::GetOrCreateLogger("RHI.Diagnostics.TimestampAllocator");

RhiTimestampQueryAllocator::RhiTimestampQueryAllocator(std::uint32_t poolCount, std::uint32_t queriesPerPool) :
	m_freeQueryIndices(poolCount)
{
	for (std::vector<std::uint32_t>& freeQueryIndices : m_freeQueryIndices)
	{
		freeQueryIndices.reserve(queriesPerPool);
		for (std::uint32_t queryIndex = queriesPerPool; queryIndex > 0; --queryIndex)
		{
			freeQueryIndices.push_back(queryIndex - 1);
		}
	}
}

RhiTimestampQueryHandle RhiTimestampQueryAllocator::Allocate(std::uint32_t poolIndex)
{
	std::lock_guard lock(m_mutex);
	if (poolIndex >= m_freeQueryIndices.size() ||
	    m_freeQueryIndices[poolIndex].empty() ||
	    m_queryLocations.size() >= std::numeric_limits<std::uint32_t>::max() - 1)
	{
		Diagnostics::Fatal(
		    g_rhiTimestampQueryAllocatorLogger,
		    __FILE__,
		    __LINE__,
		    "Timestamp query capacity was exhausted or addressed with an invalid pool index.");
	}

	std::vector<std::uint32_t>& freeQueryIndices = m_freeQueryIndices[poolIndex];
	const std::uint32_t queryIndex = freeQueryIndices.back();
	freeQueryIndices.pop_back();

	std::uint32_t handleValue = m_nextHandleValue++;
	while (handleValue == 0 || m_queryLocations.contains(handleValue))
	{
		handleValue = m_nextHandleValue++;
	}

	m_queryLocations.emplace(
	    handleValue,
	    RhiTimestampQueryLocation{.PoolIndex = poolIndex, .QueryIndex = queryIndex});
	return RhiTimestampQueryHandle{.Value = handleValue};
}

void RhiTimestampQueryAllocator::Release(RhiTimestampQueryHandle query) noexcept
{
	std::lock_guard lock(m_mutex);
	const auto locationIt = m_queryLocations.find(query.Value);
	if (locationIt == m_queryLocations.end())
	{
		Diagnostics::Fatal(g_rhiTimestampQueryAllocatorLogger, __FILE__, __LINE__, "Released an unknown timestamp query handle.");
	}

	const RhiTimestampQueryLocation location = locationIt->second;
	if (location.PoolIndex < m_freeQueryIndices.size())
	{
		m_freeQueryIndices[location.PoolIndex].push_back(location.QueryIndex);
	}
	m_queryLocations.erase(locationIt);
}

RhiTimestampQueryLocation RhiTimestampQueryAllocator::Resolve(RhiTimestampQueryHandle query) const noexcept
{
	std::lock_guard lock(m_mutex);
	const auto locationIt = m_queryLocations.find(query.Value);
	if (locationIt == m_queryLocations.end())
	{
		Diagnostics::Fatal(g_rhiTimestampQueryAllocatorLogger, __FILE__, __LINE__, "Resolved an unknown timestamp query handle.");
	}
	return locationIt->second;
}
