#pragma once

#include "Tasks/Public/TaskGraph.h"

#include <memory>

namespace Assets
{
	struct SceneLoadSharedState;
	CompiledTaskGraph BuildSceneLoadTaskGraph(const std::shared_ptr<SceneLoadSharedState>& state);
}
