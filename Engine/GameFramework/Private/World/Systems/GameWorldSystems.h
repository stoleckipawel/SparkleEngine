#pragma once

#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"

class GameWorldResourceStores;
class TaskExecutor;

namespace ECS
{
	class GameWorldState;
	bool ExecuteGameWorldSystems(
	    GameWorldState& state,
	    GameWorldResourceStores& resources,
	    TaskExecutor& executor,
	    const CameraInputIntent& cameraIntent,
	    float deltaSeconds);
}
