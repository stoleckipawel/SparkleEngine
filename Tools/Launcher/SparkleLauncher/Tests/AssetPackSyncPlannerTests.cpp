#include "AssetPackSyncPlanner.h"
#include "LevelOperationProcessRequests.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SparkleLauncher::AssetPackSyncPlannerTests
{
	class TemporaryDirectory final
	{
	public:
		TemporaryDirectory()
		{
			const auto uniqueValue = std::chrono::steady_clock::now().time_since_epoch().count();
			m_path = std::filesystem::temp_directory_path() / ("SparkleAssetPackSyncPlannerTests-" + std::to_string(uniqueValue));
			std::filesystem::create_directories(m_path);
		}

		~TemporaryDirectory() noexcept
		{
			std::error_code error;
			std::filesystem::remove_all(m_path, error);
		}

		TemporaryDirectory(const TemporaryDirectory&) = delete;
		TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

		std::filesystem::path MakeRequiredPath(std::string_view packId, bool create) const
		{
			const std::filesystem::path path = m_path / packId / "required.asset";
			if (create)
			{
				std::filesystem::create_directories(path.parent_path());
				std::ofstream(path, std::ios::binary | std::ios::trunc) << "ready\n";
			}
			return path;
		}

	private:
		std::filesystem::path m_path;
	};

	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	ProjectAssetPack MakePack(
	    const TemporaryDirectory& temporaryDirectory,
	    std::string id,
	    bool downloadSupported,
	    bool runtimeSupported,
	    bool sourceReady,
	    std::string parentPackId = {})
	{
		ProjectAssetPack pack;
		pack.id = std::move(id);
		pack.rootPath = temporaryDirectory.MakeRequiredPath(pack.id, sourceReady);
		pack.parentPackId = std::move(parentPackId);
		pack.downloadSupported = downloadSupported;
		pack.runtimeSupported = runtimeSupported;
		return pack;
	}

	ProjectLevelCatalogEntry MakeSelectedLevel(std::string id, std::string packId)
	{
		ProjectLevelCatalogEntry level;
		level.id = std::move(id);
		level.assetPackId = std::move(packId);
		level.selected = true;
		return level;
	}

	void SelectedPackPlanIsParentFirstAndDeduplicated()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("Base", MakePack(temporaryDirectory, "Base", true, true, false));
		catalog.assetPacks.emplace("Child", MakePack(temporaryDirectory, "Child", true, true, false, "Base"));
		catalog.levels.push_back(MakeSelectedLevel("ChildA", "Child"));
		catalog.levels.push_back(MakeSelectedLevel("ChildB", "Child"));

		const std::vector<std::string> plan = BuildAssetPackSyncPlan(catalog);
		Require(plan == std::vector<std::string>({"Base", "Child"}), "Selected pack plan was not parent-first and deduplicated.");
	}

	void RequestedLevelPlanIsIsolatedFromOtherSelections()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("First", MakePack(temporaryDirectory, "First", true, true, false));
		catalog.assetPacks.emplace("Second", MakePack(temporaryDirectory, "Second", true, true, false));
		catalog.levels.push_back(MakeSelectedLevel("FirstLevel", "First"));
		catalog.levels.push_back(MakeSelectedLevel("SecondLevel", "Second"));

		const std::vector<std::string> requestedLevelIds{"SecondLevel"};
		const std::vector<std::string> plan = BuildAssetPackSyncPlan(catalog, requestedLevelIds);
		Require(plan == std::vector<std::string>({"Second"}), "Targeted level sync included another selected level's pack.");
	}

	void UnselectedRequestedLevelCanBeAcquired()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("Example", MakePack(temporaryDirectory, "Example", true, true, false));
		ProjectLevelCatalogEntry level = MakeSelectedLevel("ExampleLevel", "Example");
		level.selected = false;
		catalog.levels.push_back(std::move(level));
		const std::vector<std::string> requestedLevelIds{"ExampleLevel"};

		const std::vector<std::string> plan = BuildAssetPackSyncPlan(catalog, requestedLevelIds);
		Require(plan == std::vector<std::string>({"Example"}), "An explicitly requested unselected level was not acquired.");
	}

	void RequestedRuntimeUnsupportedPackCanBeAcquired()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("Future", MakePack(temporaryDirectory, "Future", true, false, false));
		ProjectLevelCatalogEntry level = MakeSelectedLevel("FutureLevel", "Future");
		level.selected = false;
		catalog.levels.push_back(std::move(level));
		const std::vector<std::string> requestedLevelIds{"FutureLevel"};

		const std::vector<std::string> plan = BuildAssetPackSyncPlan(catalog, requestedLevelIds);
		Require(plan == std::vector<std::string>({"Future"}), "A requested source-only pack was not acquired.");
	}

	void SelectedRuntimeUnsupportedPackIsRejected()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		ProjectAssetPack futurePack = MakePack(temporaryDirectory, "Future", true, false, false);
		futurePack.runtimeBlocker = "Required runtime capability is unavailable.";
		catalog.assetPacks.emplace("Future", std::move(futurePack));
		catalog.levels.push_back(MakeSelectedLevel("FutureLevel", "Future"));

		try
		{
			BuildAssetPackSyncPlan(catalog);
		}
		catch (const Diagnostics::Error& error)
		{
			Require(std::string_view(error.what()).find("runtime-unsupported") != std::string_view::npos, error.what());
			return;
		}
		throw std::runtime_error("Selected runtime-unsupported pack was accepted.");
	}

	void SelectedPackWithoutAcquisitionPathIsRejected()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("Unavailable", MakePack(temporaryDirectory, "Unavailable", false, true, false));
		catalog.levels.push_back(MakeSelectedLevel("Unavailable", "Unavailable"));

		try
		{
			BuildAssetPackSyncPlan(catalog);
		}
		catch (const Diagnostics::Error& error)
		{
			Require(std::string_view(error.what()).find("no supported acquisition path") != std::string_view::npos, error.what());
			return;
		}
		throw std::runtime_error("Unavailable selected pack was accepted.");
	}

	void PresentPackWithoutDownloadRemainsUsable()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("Local", MakePack(temporaryDirectory, "Local", false, true, true));
		catalog.levels.push_back(MakeSelectedLevel("Local", "Local"));

		const std::vector<std::string> plan = BuildAssetPackSyncPlan(catalog);
		Require(plan == std::vector<std::string>({"Local"}), "Present local pack was not retained in the selected pack plan.");
	}

	void SourceReadinessIncludesParentChain()
	{
		TemporaryDirectory temporaryDirectory;
		ProjectLevelCatalog catalog;
		catalog.assetPacks.emplace("Base", MakePack(temporaryDirectory, "Base", true, true, false));
		catalog.assetPacks.emplace("Child", MakePack(temporaryDirectory, "Child", true, true, true, "Base"));
		Require(!catalog.IsAssetPackSourceReady("Child"), "Child source was ready while its parent payload was missing.");

		temporaryDirectory.MakeRequiredPath("Base", true);
		Require(catalog.IsAssetPackSourceReady("Child"), "Complete parent chain was not source ready.");
		Require(catalog.IsAssetPackReady("Child"), "Runtime-supported source-ready chain was not ready.");
	}

	void ExecutionPlanDriftIsRejected()
	{
		LevelOperationPlan plan;
		LevelOperationStep plannedStep;
		plannedStep.Id = "sync-asset-pack-Example";
		plannedStep.DisplayName = "Acquire Example";
		plannedStep.DisplayCommandLine = "cmake -DVALUE=one";
		plannedStep.LogPath = "Example.log";
		plan.Steps.push_back(plannedStep);

		LevelOperationProcessStep executableStep;
		executableStep.Id = plannedStep.Id;
		executableStep.DisplayName = plannedStep.DisplayName;
		executableStep.Request.ExecutablePath = "cmake";
		executableStep.Request.Arguments = {"-DVALUE=one"};
		executableStep.Request.LogPath = plannedStep.LogPath;
		std::vector<LevelOperationProcessStep> executableSteps = {executableStep};
		Require(LevelOperationExecutionPlanMatches(plan, executableSteps), "Identical execution plan was rejected.");

		executableSteps.front().Request.Arguments = {"-DVALUE=two"};
		Require(!LevelOperationExecutionPlanMatches(plan, executableSteps), "Changed process arguments were accepted after planning.");
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main()
{
	using namespace SparkleLauncher::AssetPackSyncPlannerTests;
	int failureCount = 0;
	failureCount += Run("selected pack dependency order", SelectedPackPlanIsParentFirstAndDeduplicated);
	failureCount += Run("targeted level isolation", RequestedLevelPlanIsIsolatedFromOtherSelections);
	failureCount += Run("targeted unselected level acquisition", UnselectedRequestedLevelCanBeAcquired);
	failureCount += Run("targeted source-only acquisition", RequestedRuntimeUnsupportedPackCanBeAcquired);
	failureCount += Run("runtime-unsupported selection rejection", SelectedRuntimeUnsupportedPackIsRejected);
	failureCount += Run("missing acquisition path rejection", SelectedPackWithoutAcquisitionPathIsRejected);
	failureCount += Run("present local pack", PresentPackWithoutDownloadRemainsUsable);
	failureCount += Run("parent source readiness", SourceReadinessIncludesParentChain);
	failureCount += Run("execution plan drift", ExecutionPlanDriftIsRejected);
	return failureCount == 0 ? 0 : 1;
}
