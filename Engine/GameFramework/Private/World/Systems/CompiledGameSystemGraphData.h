#pragma once

#include "World/Systems/GameSystemGraph.h"
#include "Tasks/Public/TaskGraph.h"

#include <cstdint>
#include <vector>

namespace ECS
{
	struct GameSystemWave final
	{
		std::vector<std::uint32_t> Systems;
		// This key describes the ranges captured by the single retained graph, not mutable world truth.
		std::vector<std::uint32_t> ItemCounts;
		CompiledTaskGraph Tasks;
	};

	struct CompiledGameSystemGraphData final
	{
		std::vector<GameSystemDesc> Systems;
		std::vector<GameSystemWave> Waves;
		GameSystemGraphError Error;
	};
}
