#pragma once

#include "Tasks/Public/TaskGraph.h"

#include <memory>

namespace Assets
{
	struct SceneLoadWorkState;
	CompiledTaskGraph BuildSceneLoadTaskGraph(const std::shared_ptr<SceneLoadWorkState>& state);
}
