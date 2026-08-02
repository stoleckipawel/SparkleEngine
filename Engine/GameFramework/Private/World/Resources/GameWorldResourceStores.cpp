#include "PCH.h"

#include "World/Resources/GameWorldResourceStores.h"

#include <atomic>
#include <limits>

static const auto g_gameWorldResourceStoresLogger = Logging::GetOrCreateLogger("GameFramework.ResourceStores");

std::uint32_t GameWorldResourceStores::IssueGeneration() noexcept
{
	static std::atomic_uint32_t nextGeneration{1};
	std::uint32_t generation = nextGeneration.load(std::memory_order_relaxed);
	for (;;)
	{
		if (generation == 0)
		{
			Diagnostics::Fatal(g_gameWorldResourceStoresLogger, __FILE__, __LINE__, "Game-world resource generation identity exhausted.");
		}

		const std::uint32_t next = generation == (std::numeric_limits<std::uint32_t>::max)() ? 0 : generation + 1;
		if (nextGeneration.compare_exchange_weak(generation, next, std::memory_order_relaxed, std::memory_order_relaxed))
		{
			return generation;
		}
	}
}

GameWorldResourceStores::GameWorldResourceStores() noexcept :
    Generation(IssueGeneration()),
    Materials(Generation),
    Textures(Generation)
{
}
