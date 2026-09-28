#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

class CompiledTaskGraph;
class TaskExecutionContext;
class TaskResult;
struct TaskExecutorConfig;

class AssetCookerSceneBatch final
{
public:
	using ProgressCallback = std::function<void(std::string_view, std::size_t, std::size_t)>;

	static bool Execute(
	    const std::vector<AssetCookerSceneEntry>& sceneEntries,
	    AssetCookerDiagnostics& diagnostics,
	    const ProgressCallback& progress = {});

private:
	struct Item;

	static std::uint32_t ResolveWorkerCount() noexcept;
	static TaskExecutorConfig BuildExecutorConfig(std::uint32_t taskCapacity);
	static CompiledTaskGraph BuildTaskGraph(
	    const std::vector<AssetCookerSceneEntry>& sceneEntries,
	    std::vector<Item>& items,
	    std::uint32_t taskCapacity,
	    const std::function<void()>& itemCompleted);
	static TaskResult BuildProduct(
	    const std::vector<AssetCookerSceneEntry>& sceneEntries,
	    std::vector<Item>& items,
	    std::uint32_t index,
	    TaskExecutionContext& context);
	static bool BuildProducts(
	    const std::vector<AssetCookerSceneEntry>& sceneEntries,
	    std::vector<Item>& items,
	    const ProgressCallback& progress,
	    std::size_t totalWork);
	static void MergeDiagnostics(std::vector<Item>& items, AssetCookerDiagnostics& diagnostics);
	static bool PublishProducts(std::vector<Item>& items, AssetCookerDiagnostics& diagnostics);
};
