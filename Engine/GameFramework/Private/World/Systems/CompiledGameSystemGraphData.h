#pragma once

#include "World/Systems/GameSystemGraph.h"

#include <cstdint>
#include <vector>

namespace ECS
{
	struct CompiledGameSystemGraphData final
	{
		std::vector<GameSystemDesc> Systems;
		std::vector<std::vector<std::uint32_t>> Edges;
		std::vector<std::vector<std::uint32_t>> Waves;
		GameSystemGraphError Error;
	};
}
