#pragma once

#include "../../Public/AssetCookRequest.h"
#include "../Diagnostics/AssetCookerDiagnostics.h"

#include <filesystem>
#include <string>

class AssetCookerService final
{
public:
	explicit AssetCookerService(const AssetCookerConfig* config);

	AssetCookerServiceResult CookProject(const AssetCookRequest* request);
	AssetCookerServiceResult RecookAssets(const AssetRecookRequest* request);
	AssetCookerCapabilities QueryCapabilities() const noexcept;

private:
	AssetCookerServiceResult CookCategory(
	    const char* projectName,
	    const char* configuration,
	    AssetCookerCategory category);
	bool ResolveRepositoryRoot(AssetCookerDiagnostics& diagnostics, std::filesystem::path& outRepositoryRoot) const;
	std::string ResolveProjectName(const char* requestProjectName) const;
	std::string ResolveConfiguration(const char* requestConfiguration) const;
	AssetCookerCategory ResolveRecookCategory(const AssetRecookRequest* request, AssetCookerDiagnostics& diagnostics) const;

	std::filesystem::path configuredRepositoryRoot;
	std::string configuredProjectName;
	std::string configuredConfiguration;
};
