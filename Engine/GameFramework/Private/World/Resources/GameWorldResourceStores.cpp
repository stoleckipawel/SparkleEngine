#include "PCH.h"

#include "World/Resources/GameWorldResourceStores.h"

#include <atomic>

class GameWorldResourceStoresOperations final
{
  public:
	inline static std::atomic<std::uint32_t> g_nextResourceGeneration = 1;

	static std::uint32_t AcquireResourceGeneration() noexcept
	{
		std::uint32_t generation = g_nextResourceGeneration.fetch_add(1, std::memory_order_relaxed);
		if (generation == 0)
			generation = g_nextResourceGeneration.fetch_add(1, std::memory_order_relaxed);
		return generation;
	}
};

GameWorldResourceStores::GameWorldResourceStores() noexcept :
    Generation(GameWorldResourceStoresOperations::AcquireResourceGeneration()), Materials(Generation), Textures(Generation)
{
}
