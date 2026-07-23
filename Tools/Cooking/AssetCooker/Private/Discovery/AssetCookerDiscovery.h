#pragma once

#include "../Diagnostics/AssetCookerDiagnostics.h"
#include "../Planning/ProjectCookPlan.h"

#include <filesystem>
#include <set>
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
	enum class CatalogSection;
	struct CatalogLevel;

	static bool CollectCatalogDefaultSceneIds(
	    const std::filesystem::path& projectRoot,
	    std::set<std::string>& outSceneIds);
};
