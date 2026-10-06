#pragma once

#include "Tasks/Public/TaskTypes.h"

class TaskExecutionContext;
struct RenderScenePreparationRun;

namespace RenderScenePreparationMerger
{
	TaskResult Merge(TaskExecutionContext& context);
	TaskResult BuildRayTracingPlan(TaskExecutionContext& context);
	void PublishSceneOutputs(RenderScenePreparationRun& run);

}
