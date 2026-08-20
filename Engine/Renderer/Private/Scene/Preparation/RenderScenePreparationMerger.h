#pragma once

#include "Tasks/Public/TaskTypes.h"

class TaskExecutionContext;
struct RenderScenePreparationRun;

class RenderScenePreparationMerger final
{
public:
	static TaskResult Merge(TaskExecutionContext& context);
	static TaskResult BuildRayTracingPlan(TaskExecutionContext& context);
	static void PublishSceneOutputs(RenderScenePreparationRun& run);

private:
	static void PublishPrimitives(RenderScenePreparationRun& run);
};
