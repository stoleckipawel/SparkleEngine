#include "AssetCookerSceneBatch.h"

#include "CookedSceneGenerationWriter.h"
#include "ImportedSceneCooker.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <thread>
#include <utility>

struct AssetCookerSceneBatch::Item final
{
	ImportedSceneCookProduct Product;
	AssetCookerDiagnostics Diagnostics;
	bool Built = false;
};

bool AssetCookerSceneBatch::Execute(
    const std::vector<AssetCookerSceneEntry>& sceneEntries,
    AssetCookerDiagnostics& diagnostics)
{
	std::vector<Item> items(sceneEntries.size());
	const bool built = BuildProducts(sceneEntries, items);

	MergeDiagnostics(items, diagnostics);
	if (!built)
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    "Scene asset generation failed before publication.");
		return false;
	}

	return PublishProducts(items, diagnostics);
}

std::uint32_t AssetCookerSceneBatch::ResolveWorkerCount() noexcept
{
	const std::uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	return std::clamp(hardwareThreads > 1u ? hardwareThreads - 1u : 1u, 1u, 4u);
}

bool AssetCookerSceneBatch::BuildProducts(
    const std::vector<AssetCookerSceneEntry>& sceneEntries,
    std::vector<Item>& items)
{
	const std::uint32_t taskCapacity =
	    static_cast<std::uint32_t>(std::max<std::size_t>(sceneEntries.size(), 1u));

	TaskExecutor executor(
	    TaskExecutorConfig{
	        .FrameCriticalWorkerCount = 1u,
	        .BackgroundWorkerCount = ResolveWorkerCount(),
	        .MaximumTasksPerExecution = taskCapacity,
	        .MaximumEdgesPerExecution = 1u,
	        .MaximumActiveExecutions = 1u});

	TaskGraphBuilder builder(
	    TaskGraphLimits{
	        .MaximumTasks = taskCapacity,
	        .MaximumEdges = 1u});

	for (std::uint32_t index = 0; index < sceneEntries.size(); ++index)
	{
		builder.Add(
		    TaskDesc{
		        .Name = TaskName("Build cataloged scene"),
		        .Lane = TaskLane::Background},
		    [&sceneEntries, &items, index](TaskExecutionContext& context)
		    {
			    if (context.IsCancellationRequested())
			    {
				    return TaskResult::Cancelled("Scene asset generation was cancelled.");
			    }

			    Item& item = items[index];
			    item.Built = ImportedSceneCooker::Build(
			        sceneEntries[index],
			        item.Diagnostics,
			        item.Product);

			    return item.Built
			               ? TaskResult::Success()
			               : TaskResult::Failure("Cataloged scene generation failed.");
		    });
	}

	TaskExecutionContext context;
	const TaskExecution execution = executor.Submit(builder.Compile(), context);
	return execution.GetStatus() == TaskExecutionStatus::Succeeded;
}

void AssetCookerSceneBatch::MergeDiagnostics(
    std::vector<Item>& items,
    AssetCookerDiagnostics& diagnostics)
{
	for (Item& item : items)
	{
		diagnostics.Append(item.Diagnostics.ReleaseRecords());
	}
}

bool AssetCookerSceneBatch::PublishProducts(
    std::vector<Item>& items,
    AssetCookerDiagnostics& diagnostics)
{
	std::vector<const CookedSceneBuild*> builds;
	builds.reserve(items.size());
	for (const Item& item : items)
	{
		builds.push_back(&item.Product.Scene);
	}

	std::string errorMessage;
	if (!CookedSceneGenerationWriter::Publish(builds, errorMessage))
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    std::move(errorMessage));
		return false;
	}

	return true;
}
