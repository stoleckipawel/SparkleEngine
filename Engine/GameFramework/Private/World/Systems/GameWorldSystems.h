#pragma once

#include "World/Systems/CameraSimulationInput.h"
#include "World/Systems/Execution/AnimationSystemExecution.h"
#include "World/Systems/Execution/MeshExtractionSystemExecution.h"
#include "World/Systems/Execution/SimulationSystemExecution.h"
#include "World/Systems/Execution/TransformSystemExecution.h"

class GameWorldResourceStores;
class TaskExecutor;

namespace ECS
{
	class CompiledGameSystemGraph;
	CompiledGameSystemGraph BuildGameWorldSystemGraph();

	class GameWorldState;

	struct GameWorldSystemExecutionContext final
	{
		GameWorldResourceStores& Resources;
		TaskExecutor& Executor;
		CameraSimulationInput Camera;
	};

	class GameWorldSystemExecution final
	{
	public:
		GameWorldSystemExecution(GameWorldState& state, const GameWorldSystemExecutionContext& context, const StructureFrozenEpoch& epoch);
		static CompiledGameSystemGraph BuildGraph();

	private:
		SimulationSystemExecution m_simulation;
		AnimationSystemExecution m_animation;
		TransformSystemExecution m_transform;
		MeshExtractionSystemExecution m_extraction;
		GameWorldState& m_state;
	};

	bool ExecuteGameWorldSystems(GameWorldState& state, const GameWorldSystemExecutionContext& context);
}
