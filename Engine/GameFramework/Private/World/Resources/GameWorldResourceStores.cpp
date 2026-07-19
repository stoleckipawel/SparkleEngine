#include "PCH.h"

#include "World/Resources/GameWorldResourceStores.h"

#include <atomic>

namespace
{
	std::atomic<std::uint32_t> g_nextResourceGeneration = 1;

	std::uint32_t AcquireResourceGeneration() noexcept
	{
		std::uint32_t generation = g_nextResourceGeneration.fetch_add(1, std::memory_order_relaxed);
		if (generation == 0)
			generation = g_nextResourceGeneration.fetch_add(1, std::memory_order_relaxed);
		return generation;
	}
}

GameWorldResourceStores::GameWorldResourceStores() noexcept :
    Generation(AcquireResourceGeneration()), Materials(Generation), Textures(Generation)
{
}
