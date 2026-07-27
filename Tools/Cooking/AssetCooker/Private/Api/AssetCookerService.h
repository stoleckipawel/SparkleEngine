#pragma once

#include "AssetCookerTypes.h"
#include "../Diagnostics/AssetCookerDiagnostics.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class AssetCookerService final
{
  public:
	AssetCookerService(const char* repositoryRoot, const char* projectName, const char* configuration);

	AssetCookerServiceResult Cook(const char* projectName, const char* configuration, AssetCookerCategory category);

  private:
	static bool HasText(const char* text) noexcept;
	static bool IsAllProjects(std::string_view projectName) noexcept;
	static AssetCookerServiceResult Finish(
	    bool succeeded,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord> outputs = {});

	bool ResolveRepositoryRoot(AssetCookerDiagnostics& diagnostics, std::filesystem::path& outRepositoryRoot) const;
	bool ResolveProjects(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<std::string>& outProjects) const;
	bool CookProjects(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view configuration,
	    AssetCookerCategory category,
	    const std::vector<std::string>& projects,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outOutputs) const;
	bool CookProject(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectName,
	    std::string_view configuration,
	    AssetCookerCategory category,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outOutputs) const;
	std::string ResolveProjectName(const char* requestProjectName) const;
	std::string ResolveConfiguration(const char* requestConfiguration) const;

	std::filesystem::path m_configuredRepositoryRoot;
	std::string m_configuredProjectName;
	std::string m_configuredConfiguration;
};
