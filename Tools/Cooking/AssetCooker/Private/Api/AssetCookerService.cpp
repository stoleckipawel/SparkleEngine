#include "AssetCookerService.h"

#include "../Discovery/AssetCookerDiscovery.h"
#include "../Dispatch/AssetCookerDispatcher.h"

AssetCookerService::AssetCookerService(const char* repositoryRoot, const char* projectName, const char* configuration)
{
	if (HasText(repositoryRoot))
	{
		configuredRepositoryRoot = std::filesystem::path(repositoryRoot);
	}
	if (HasText(projectName))
	{
		configuredProjectName = projectName;
	}
	if (HasText(configuration))
	{
		configuredConfiguration = configuration;
	}
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
	if (IsAllProjects(resolvedProjectName))
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

bool AssetCookerService::HasText(const char* text) noexcept
{
	return text != nullptr && text[0] != '\0';
}

bool AssetCookerService::IsAllProjects(std::string_view projectName) noexcept
{
	return projectName.empty() || projectName == "ALL" || projectName == "All" || projectName == "all";
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
	if (HasText(requestProjectName))
	{
		return requestProjectName;
	}
	return configuredProjectName;
}

std::string AssetCookerService::ResolveConfiguration(const char* requestConfiguration) const
{
	if (HasText(requestConfiguration))
	{
		return requestConfiguration;
	}
	if (!configuredConfiguration.empty())
	{
		return configuredConfiguration;
	}
	return "DevelopmentGame";
}
