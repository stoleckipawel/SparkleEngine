#pragma once

#include "RhiQueue.h"

#include <array>

struct RhiQueueCapability final
{
	bool Supported = false;
	bool Independent = false;
};

struct RhiQueueCapabilities final
{
	constexpr void Set(ERhiQueueType queue, bool supported, bool independent) noexcept
	{
		if (queue == ERhiQueueType::Count)
		{
			return;
		}

		Entries[RhiQueueTypeToIndex(queue)] = RhiQueueCapability{
		    .Supported = supported,
		    .Independent = supported && independent};
	}

	constexpr RhiQueueCapability Get(ERhiQueueType queue) const noexcept
	{
		return queue != ERhiQueueType::Count ? Entries[RhiQueueTypeToIndex(queue)] : RhiQueueCapability{};
	}

	constexpr bool Supports(ERhiQueueType queue) const noexcept { return Get(queue).Supported; }
	constexpr bool IsIndependent(ERhiQueueType queue) const noexcept { return Get(queue).Independent; }
	constexpr bool SupportsIndependent(ERhiQueueType queue) const noexcept
	{
		const RhiQueueCapability capability = Get(queue);
		return capability.Supported && capability.Independent;
	}

	std::array<RhiQueueCapability, RhiQueueTypeCount> Entries{};
};
