#pragma once

#include "../Diagnostics/AssetCookerDiagnostics.h"
#include "../Planning/ProjectCookPlan.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class AssetCookerDiscovery final
{
  public:
	static bool TryFindRepositoryRoot(const std::filesystem::path& startPath, std::filesystem::path& outRepositoryRoot);
	static bool ValidateConfiguration(std::string_view configuration);
	static std::vector<std::string> DiscoverProjects(
	    const std::filesystem::path& repositoryRoot,
	    AssetCookerDiagnostics& diagnostics);
	static bool BuildProjectCookPlan(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    std::string_view configuration,
	    AssetCookerCategory category,
	    AssetCookerProjectCookPlan& outPlan,
	    AssetCookerDiagnostics& diagnostics);

  private:
	static bool PathExists(const std::filesystem::path& path);
	static bool CategoryNeedsScenes(AssetCookerCategory category) noexcept;
	static void AddPlanSteps(
	    AssetCookerCategory category,
	    std::vector<AssetCookerPlanStep>& outSteps);
	static std::string ResolveToolConfiguration(std::string_view configuration);
	static bool CollectSceneIds(
	    const std::filesystem::path& projectRoot,
	    std::vector<std::string>& outSceneIds,
	    AssetCookerDiagnostics& diagnostics);
	static bool ResolveSceneEntry(
	    const std::filesystem::path& projectRoot,
	    std::string_view sceneId,
	    AssetCookerSceneEntry& outEntry,
	    AssetCookerDiagnostics& diagnostics);
};
