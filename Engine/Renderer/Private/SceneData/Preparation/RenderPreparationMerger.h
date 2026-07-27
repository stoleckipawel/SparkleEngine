#pragma once

#include "Tasks/Public/TaskTypes.h"

#include <span>

class TaskExecutionContext;
struct MeshRenderItem;
struct RenderPreparationRun;
struct RenderSceneData;

class RenderPreparationMerger final
{
  public:
	static TaskResult Merge(TaskExecutionContext& context);
	static TaskResult BuildRayTracingPlan(TaskExecutionContext& context);
	static void PublishFrameOutputs(RenderPreparationRun& run);

  private:
	static void PublishObjects(RenderPreparationRun& run);
	static void PublishBatches(RenderPreparationRun& run, std::span<const MeshRenderItem> renderItems);
	static void PublishWorkload(RenderSceneData& sceneData) noexcept;
};
