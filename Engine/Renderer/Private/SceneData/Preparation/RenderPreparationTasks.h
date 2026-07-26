#pragma once

#include "Tasks/Public/TaskTypes.h"

#include <cstdint>

class TaskExecutionContext;

class RenderPreparationTasks final
{
  public:
	static TaskResult TransformObjects(
	    std::uint32_t begin,
	    std::uint32_t end,
	    TaskExecutionContext& context);
	static TaskResult EvaluateVisibility(
	    std::uint32_t begin,
	    std::uint32_t end,
	    TaskExecutionContext& context);
	static TaskResult CopySkinning(
	    std::uint32_t begin,
	    std::uint32_t end,
	    TaskExecutionContext& context);
	static TaskResult CopyMorph(
	    std::uint32_t begin,
	    std::uint32_t end,
	    TaskExecutionContext& context);
	static TaskResult PrepareLights(
	    std::uint32_t begin,
	    std::uint32_t end,
	    TaskExecutionContext& context);
};
