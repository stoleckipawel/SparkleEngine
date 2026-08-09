#pragma once

#include "../Diagnostics/AssetCookerDiagnostics.h"
#include "../Planning/ProjectCookPlan.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct ProjectLevelCatalog;
struct ProjectLevelCatalogEntry;

class AssetCookerDiscovery final
{
public:
	static bool TryFindRepositoryRoot(const std::filesystem::path& startPath, std::filesystem::path& outRepositoryRoot);
	static std::optional<std::string_view> ResolveToolProfile(std::string_view configuration) noexcept;
	static std::vector<std::string> DiscoverProjects(const std::filesystem::path& repositoryRoot, AssetCookerDiagnostics& diagnostics);
	static bool BuildProjectCookPlan(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    std::string_view configuration,
	    std::string_view toolProfile,
	    AssetCookerCategory category,
	    AssetCookerProjectCookPlan& outPlan,
	    AssetCookerDiagnostics& diagnostics);

private:
	static bool PathExists(const std::filesystem::path& path);
	static bool CategoryNeedsScenes(AssetCookerCategory category) noexcept;
	static void InitializePlan(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    std::string_view configuration,
	    std::string_view toolProfile,
	    AssetCookerCategory category,
	    AssetCookerProjectCookPlan& outPlan);
	static void AddPlanSteps(AssetCookerCategory category, std::vector<AssetCookerPlanStep>& outSteps);
	static bool CollectSceneEntries(
	    const std::filesystem::path& projectRoot,
	    std::vector<AssetCookerSceneEntry>& outEntries,
	    AssetCookerDiagnostics& diagnostics);
	static bool CollectSceneIds(
	    const std::filesystem::path& projectRoot,
	    std::vector<std::string>& outSceneIds,
	    AssetCookerDiagnostics& diagnostics);
	static bool AppendLevelSceneIds(
	    const ProjectLevelCatalog& catalog,
	    const ProjectLevelCatalogEntry& level,
	    std::vector<std::string>& outSceneIds,
	    AssetCookerDiagnostics& diagnostics);
	static bool ResolveSceneEntry(
	    const std::filesystem::path& projectRoot,
	    std::string_view sceneId,
	    AssetCookerSceneEntry& outEntry,
	    AssetCookerDiagnostics& diagnostics);
	static bool IsSceneIdSafe(const std::filesystem::path& relativeScenePath) noexcept;
	static bool ResolveSceneSource(
	    const std::filesystem::path& meshRoot,
	    const std::filesystem::path& relativeScenePath,
	    std::string_view sceneId,
	    std::filesystem::path& outSourcePath,
	    AssetCookerDiagnostics& diagnostics);
};
