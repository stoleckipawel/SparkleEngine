#include "AssetCookerService.h"

#include "../Discovery/AssetCookerDiscovery.h"
#include "../Dispatch/AssetCookerDispatcher.h"

#include <utility>

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
		return Finish(false, diagnostics);
	}

	const std::string resolvedConfiguration = ResolveConfiguration(configuration);
	if (!AssetCookerDiscovery::ValidateConfiguration(resolvedConfiguration))
	{
		diagnostics.AddError(AssetCookerCategory_All, "Unsupported configuration '" + resolvedConfiguration + "'.");
		return Finish(false, diagnostics);
	}

	std::vector<std::string> projects;
	const std::string resolvedProjectName = ResolveProjectName(projectName);
	if (IsAllProjects(resolvedProjectName))
	{
		projects = AssetCookerDiscovery::DiscoverProjects(repositoryRoot, diagnostics);
		if (projects.empty())
		{
			return Finish(false, diagnostics);
		}
	}
	else
	{
		projects.push_back(resolvedProjectName);
	}

	std::vector<AssetCookerOutputRecord> outputs;
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
			return Finish(false, diagnostics, std::move(outputs));
		}

		if (!AssetCookerDispatcher::ValidateCapabilities(plan, diagnostics))
		{
			return Finish(false, diagnostics, std::move(outputs));
		}

		if (!AssetCookerDispatcher::DispatchPlan(plan, diagnostics, outputs))
		{
			return Finish(false, diagnostics, std::move(outputs));
		}
	}

	return Finish(true, diagnostics, std::move(outputs));
}

bool AssetCookerService::HasText(const char* text) noexcept
{
	return text != nullptr && text[0] != '\0';
}

bool AssetCookerService::IsAllProjects(std::string_view projectName) noexcept
{
	return projectName.empty() || projectName == "ALL" || projectName == "All" || projectName == "all";
}

AssetCookerServiceResult AssetCookerService::Finish(
    bool succeeded,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord> outputs)
{
	AssetCookerServiceResult result;
	result.succeeded = succeeded;
	result.exitCode = succeeded ? 0 : 1;
	result.diagnostics = diagnostics.ReleaseRecords();
	result.outputs = std::move(outputs);
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
