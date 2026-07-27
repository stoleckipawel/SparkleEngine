#pragma once

#include "Graph/TaskGraphStorage.h"

class TaskExecutionContext;

class TaskFunctionInvoker final
{
  public:
	static TaskResult Invoke(const TaskGraphNode& node, TaskExecutionContext& context);
};
