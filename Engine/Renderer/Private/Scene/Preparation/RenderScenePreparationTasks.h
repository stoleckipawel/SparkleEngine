#pragma once

#include "Tasks/Public/TaskTypes.h"

#include <cstdint>

class TaskExecutionContext;

namespace RenderScenePreparationTasks
{
	TaskResult TransformPrimitives(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	TaskResult CopyJointMatrices(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	TaskResult CopyMorphWeights(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
	TaskResult PrepareLights(std::uint32_t begin, std::uint32_t end, TaskExecutionContext& context);
}
