#pragma once

#include "Tasks/Public/TaskTypes.h"

#include <cstdint>

class TaskExecutionContext;

class RenderScenePreparationTasks final
{
public:
	static TaskResult TransformPrimitives(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	static TaskResult CopyJointMatrices(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	static TaskResult CopyMorphWeights(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	static TaskResult PrepareLights(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
};
