#include "AssetCookerSceneBatch.h"

#include "CookedSceneGenerationWriter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "ImportedSceneCooker.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <mutex>
#include <thread>

struct AssetCookerSceneBatch::Item final
{
	CookedSceneBuild Build;
	AssetCookerDiagnostics Diagnostics;
};

bool AssetCookerSceneBatch::Execute(
    const std::vector<AssetCookerSceneEntry>& sceneEntries,
    AssetCookerDiagnostics& diagnostics,
    const ToolWorkProgressCallback& progress)
{
	const std::size_t totalWork = sceneEntries.size() + 1;
	if (progress)
	{
		progress({.Action = "Cooking scene assets", .Total = totalWork});
	}
	std::vector<Item> items(sceneEntries.size());
	const bool built = BuildProducts(sceneEntries, items, progress, totalWork);

	MergeDiagnostics(items, diagnostics);
	if (!built)
	{
		diagnostics.AddError(AssetCookerCategory::SceneAssets, "Scene asset generation failed before publication.");
		return false;
	}

	if (progress)
	{
		progress({.Action = "Publishing cooked scene assets", .Completed = sceneEntries.size(), .Total = totalWork});
	}
	const bool published = PublishProducts(items, diagnostics);
	if (published && progress)
	{
		progress({.Action = "Cooked scene assets published", .Completed = totalWork, .Total = totalWork});
	}
	return published;
}

std::uint32_t AssetCookerSceneBatch::ResolveWorkerCount() noexcept
{
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	return std::clamp(hardwareThreads > 1u ? hardwareThreads - 1u : 1u, 1u, 4u);
}

TaskExecutorConfig AssetCookerSceneBatch::BuildExecutorConfig(std::uint32_t taskCapacity)
{
	return TaskExecutorConfig{
	    .FrameCriticalWorkerCount = 1u,
	    .BackgroundWorkerCount = ResolveWorkerCount(),
	    .MaximumTasksPerExecution = taskCapacity,
	    .MaximumEdgesPerExecution = 1u,
	    .MaximumActiveExecutions = 1u};
}

CompiledTaskGraph AssetCookerSceneBatch::BuildTaskGraph(
    const std::vector<AssetCookerSceneEntry>& sceneEntries,
    std::vector<Item>& items,
    std::uint32_t taskCapacity,
    const std::function<void(std::uint32_t)>& itemCompleted)
{
	TaskGraphBuilder builder(TaskGraphLimits{.MaximumTasks = taskCapacity, .MaximumEdges = 1u});

	for (std::uint32_t index = 0; index < sceneEntries.size(); ++index)
	{
		builder.Add(
		    TaskDesc{.Name = TaskName("Build cataloged scene"), .Lane = TaskLane::Background},
		    [&sceneEntries, &items, &itemCompleted, index](TaskExecutionContext& context)
		    {
			    TaskResult result = BuildProduct(sceneEntries, items, index, context);
			    if (result.GetOutcome() == TaskOutcome::Succeeded)
			    {
				    itemCompleted(index);
			    }
			    return result;
		    });
	}

	return builder.Compile();
}

TaskResult AssetCookerSceneBatch::BuildProduct(
    const std::vector<AssetCookerSceneEntry>& sceneEntries,
    std::vector<Item>& items,
    std::uint32_t index,
    TaskExecutionContext& context)
{
	if (context.IsCancellationRequested())
	{
		return TaskResult::Cancelled("Scene asset generation was cancelled.");
	}

	Item& item = items[index];
	try
	{
		item.Build = ImportedSceneCooker::Build(sceneEntries[index], item.Diagnostics);
		return TaskResult::Success();
	}
	catch (const Diagnostics::Error&)
	{
		return TaskResult::Failure("Cataloged scene generation failed.");
	}
}

bool AssetCookerSceneBatch::BuildProducts(
    const std::vector<AssetCookerSceneEntry>& sceneEntries,
    std::vector<Item>& items,
    const ToolWorkProgressCallback& progress,
    std::size_t totalWork)
{
	const std::uint32_t taskCapacity = static_cast<std::uint32_t>(std::max<std::size_t>(sceneEntries.size(), 1u));
	std::mutex progressMutex;
	std::size_t completed = 0;
	const auto itemCompleted = [&sceneEntries, &progress, &progressMutex, &completed, totalWork](std::uint32_t index)
	{
		if (progress)
		{
			std::lock_guard lock(progressMutex);
			progress({.Action = "Cooked scene", .Item = sceneEntries[index].relativePath, .Completed = ++completed, .Total = totalWork});
		}
	};

	TaskExecutor executor(BuildExecutorConfig(taskCapacity));
	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(BuildTaskGraph(sceneEntries, items, taskCapacity, itemCompleted), context);
	return execution.GetStatus() == TaskExecutionStatus::Succeeded;
}

void AssetCookerSceneBatch::MergeDiagnostics(std::vector<Item>& items, AssetCookerDiagnostics& diagnostics)
{
	for (Item& item : items)
	{
		diagnostics.Append(item.Diagnostics.ReleaseRecords());
	}
}

bool AssetCookerSceneBatch::PublishProducts(std::vector<Item>& items, AssetCookerDiagnostics& diagnostics)
{
	std::vector<const CookedSceneBuild*> builds;
	builds.reserve(items.size());
	for (const Item& item : items)
	{
		builds.push_back(&item.Build);
	}

	try
	{
		CookedSceneGenerationWriter::Publish(builds);
		return true;
	}
	catch (const Diagnostics::Error& error)
	{
		diagnostics.AddError(AssetCookerCategory::SceneAssets, error.what());
		return false;
	}
}
