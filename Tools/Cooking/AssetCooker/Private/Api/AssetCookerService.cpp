#include "AssetCookerService.h"

#include "../Discovery/AssetCookerDiscovery.h"
#include "../Dispatch/AssetCookerDispatcher.h"

#include <iostream>

static bool AssetCookerHasText(const char* text)
{
	return text != nullptr && text[0] != '\0';
}

static bool AssetCookerIsAllProjects(std::string_view projectName)
{
	return projectName.empty() || projectName == "ALL" || projectName == "All" || projectName == "all";
}

AssetCookerService::AssetCookerService(const char* repositoryRoot, const char* projectName, const char* configuration)
{
	if (AssetCookerHasText(repositoryRoot))
	{
		configuredRepositoryRoot = std::filesystem::path(repositoryRoot);
	}
	if (AssetCookerHasText(projectName))
	{
		configuredProjectName = projectName;
	}
	if (AssetCookerHasText(configuration))
	{
		configuredConfiguration = configuration;
	}
}

AssetCookerCapabilities AssetCookerService::QueryCapabilities() const noexcept
{
	AssetCookerCapabilities capabilities = {};
	capabilities.supportsProjectCook = 1;
	capabilities.supportsSelectedRecook = 1;
	capabilities.supportsShaderCook = 1;
	capabilities.supportsTextureCook = 1;
	capabilities.supportsSceneAssetCook = 1;
	capabilities.supportsHotReloadOutputs = 1;
	return capabilities;
}

AssetCookerServiceResult AssetCookerService::Cook(
    const char* projectName,
    const char* configuration,
    AssetCookerCategory category)
{
	AssetCookerDiagnostics diagnostics;
	std::filesystem::path repositoryRoot;
	if (!ResolveRepositoryRoot(diagnostics, repositoryRoot))
	{
		AssetCookerServiceResult result;
		result.succeeded = false;
		result.exitCode = 1;
		result.diagnostics = diagnostics.ReleaseRecords();
		return result;
	}

	const std::string resolvedConfiguration = ResolveConfiguration(configuration);
	if (!AssetCookerDiscovery::ValidateConfiguration(resolvedConfiguration))
	{
		diagnostics.AddError(AssetCookerCategory_All, "Unsupported configuration '" + resolvedConfiguration + "'.");
		AssetCookerServiceResult result;
		result.succeeded = false;
		result.exitCode = 1;
		result.diagnostics = diagnostics.ReleaseRecords();
		return result;
	}

	std::vector<std::string> projects;
	const std::string resolvedProjectName = ResolveProjectName(projectName);
	if (AssetCookerIsAllProjects(resolvedProjectName))
	{
		projects = AssetCookerDiscovery::DiscoverProjects(repositoryRoot, diagnostics);
		if (projects.empty())
		{
			AssetCookerServiceResult result;
			result.succeeded = false;
			result.exitCode = 1;
			result.diagnostics = diagnostics.ReleaseRecords();
			return result;
		}
	}
	else
	{
		projects.push_back(resolvedProjectName);
	}

	AssetCookerServiceResult result;
	result.succeeded = true;
	result.exitCode = 0;
	for (const std::string& currentProjectName : projects)
	{
		AssetCookerProjectCookPlan plan;
		if (!AssetCookerDiscovery::BuildProjectCookPlan(
		        repositoryRoot,
		        currentProjectName,
		        resolvedConfiguration,
		        category,
		        plan,
		        diagnostics))
		{
			result.succeeded = false;
			result.exitCode = 1;
			break;
		}

		if (!AssetCookerDispatcher::ValidateCapabilities(plan, diagnostics))
		{
			result.succeeded = false;
			result.exitCode = 1;
			break;
		}

		if (!AssetCookerDispatcher::DispatchPlan(plan, diagnostics, result.outputs))
		{
			result.succeeded = false;
			result.exitCode = 1;
			break;
		}
	}

	result.diagnostics = diagnostics.ReleaseRecords();
	return result;
}

bool AssetCookerService::ResolveRepositoryRoot(
    AssetCookerDiagnostics& diagnostics,
    std::filesystem::path& outRepositoryRoot) const
{
	if (!configuredRepositoryRoot.empty())
	{
		outRepositoryRoot = std::filesystem::absolute(configuredRepositoryRoot).lexically_normal();
		return true;
	}

	std::error_code currentPathError;
	const std::filesystem::path currentPath = std::filesystem::current_path(currentPathError);
	if (currentPathError || !AssetCookerDiscovery::TryFindRepositoryRoot(currentPath, outRepositoryRoot))
	{
		diagnostics.AddError(AssetCookerCategory_All, "Failed to resolve repository root. Pass --root or set repositoryRoot.");
		return false;
	}

	return true;
}

std::string AssetCookerService::ResolveProjectName(const char* requestProjectName) const
{
	if (AssetCookerHasText(requestProjectName))
	{
		return requestProjectName;
	}
	return configuredProjectName;
}

std::string AssetCookerService::ResolveConfiguration(const char* requestConfiguration) const
{
	if (AssetCookerHasText(requestConfiguration))
	{
		return requestConfiguration;
	}
	if (!configuredConfiguration.empty())
	{
		return configuredConfiguration;
	}
	return "DevelopmentGame";
}
