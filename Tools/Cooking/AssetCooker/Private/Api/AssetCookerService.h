#pragma once

#include "AssetCookerTypes.h"
#include "../Diagnostics/AssetCookerDiagnostics.h"

#include <filesystem>
#include <string>

class AssetCookerService final
{
  public:
	AssetCookerService(const char* repositoryRoot, const char* projectName, const char* configuration);

	AssetCookerServiceResult Cook(const char* projectName, const char* configuration, AssetCookerCategory category);

  private:
	static bool HasText(const char* text) noexcept;
	static bool IsAllProjects(std::string_view projectName) noexcept;

	bool ResolveRepositoryRoot(AssetCookerDiagnostics& diagnostics, std::filesystem::path& outRepositoryRoot) const;
	std::string ResolveProjectName(const char* requestProjectName) const;
	std::string ResolveConfiguration(const char* requestConfiguration) const;

	std::filesystem::path configuredRepositoryRoot;
	std::string configuredProjectName;
	std::string configuredConfiguration;
};
