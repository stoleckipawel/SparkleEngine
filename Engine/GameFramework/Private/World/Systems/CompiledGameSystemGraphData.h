#pragma once

#include "World/Systems/GameSystemGraph.h"

#include "Tasks/Public/TaskGraph.h"

#include <cstdint>
#include <vector>

namespace ECS
{
	struct CompiledGameSystemGraphData final
	{
		std::vector<GameSystemDesc> Systems;
		std::vector<std::vector<std::uint32_t>> Edges;
		CompiledTaskGraph Tasks;
		GameSystemGraphError Error;
	};
}
