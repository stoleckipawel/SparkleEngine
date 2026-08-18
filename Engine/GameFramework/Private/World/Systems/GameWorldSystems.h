#pragma once

#include "World/Systems/CameraSimulationInput.h"

class GameWorldResourceStores;
class TaskExecutor;

namespace ECS
{
	class GameWorldState;

	struct GameWorldSystemExecutionContext final
	{
		GameWorldResourceStores& Resources;
		TaskExecutor& Executor;
		CameraSimulationInput Camera;
	};

	bool ExecuteGameWorldSystems(GameWorldState& state, const GameWorldSystemExecutionContext& context);
}
