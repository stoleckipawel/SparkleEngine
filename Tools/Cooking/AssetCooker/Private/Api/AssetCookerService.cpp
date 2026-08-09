#include "AssetCookerService.h"

#include "../Discovery/AssetCookerDiscovery.h"
#include "../Dispatch/AssetCookerDispatcher.h"

#include <optional>
#include <utility>

AssetCookerService::AssetCookerService(
    const char* repositoryRoot,
    const char* projectName,
    const char* configuration,
    const char* toolProfile)
{
	if (HasText(repositoryRoot))
	{
		m_configuredRepositoryRoot = std::filesystem::path(repositoryRoot);
	}
	if (HasText(projectName))
	{
		m_configuredProjectName = projectName;
	}
	if (HasText(configuration))
	{
		m_configuredConfiguration = configuration;
	}
	if (HasText(toolProfile))
	{
		m_configuredToolProfile = toolProfile;
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
	const std::optional<std::string_view> expectedToolProfile = AssetCookerDiscovery::ResolveToolProfile(resolvedConfiguration);
	if (!expectedToolProfile.has_value())
	{
		diagnostics.AddError(AssetCookerCategory_All, "Unsupported configuration '" + resolvedConfiguration + "'.");
		return Finish(false, diagnostics);
	}
	const std::string resolvedToolProfile = ResolveToolProfile(*expectedToolProfile);
	if (resolvedToolProfile != *expectedToolProfile)
	{
		diagnostics.AddError(
		    AssetCookerCategory_All,
		    "Tool profile '" + resolvedToolProfile + "' does not match runtime profile '" + resolvedConfiguration + "'.");
		return Finish(false, diagnostics);
	}

	const std::string resolvedProjectName = ResolveProjectName(projectName);
	std::vector<std::string> projects;
	if (!ResolveProjects(repositoryRoot, resolvedProjectName, diagnostics, projects))
	{
		return Finish(false, diagnostics);
	}

	std::vector<AssetCookerOutputRecord> outputs;
	const bool succeeded =
	    CookProjects(repositoryRoot, resolvedConfiguration, resolvedToolProfile, category, projects, diagnostics, outputs);
	return Finish(succeeded, diagnostics, std::move(outputs));
}

bool AssetCookerService::ResolveProjects(
    const std::filesystem::path& repositoryRoot,
    std::string_view projectName,
    AssetCookerDiagnostics& diagnostics,
    std::vector<std::string>& outProjects) const
{
	if (!IsAllProjects(projectName))
	{
		outProjects.emplace_back(projectName);
		return true;
	}

	outProjects = AssetCookerDiscovery::DiscoverProjects(repositoryRoot, diagnostics);
	return !outProjects.empty();
}

bool AssetCookerService::CookProjects(
    const std::filesystem::path& repositoryRoot,
    std::string_view configuration,
    std::string_view toolProfile,
    AssetCookerCategory category,
    const std::vector<std::string>& projects,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs) const
{
	for (const std::string& projectName : projects)
	{
		if (!CookProject(repositoryRoot, projectName, configuration, toolProfile, category, diagnostics, outOutputs))
		{
			return false;
		}
	}

	return true;
}

bool AssetCookerService::CookProject(
    const std::filesystem::path& repositoryRoot,
    std::string_view projectName,
    std::string_view configuration,
    std::string_view toolProfile,
    AssetCookerCategory category,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs) const
{
	AssetCookerProjectCookPlan plan;
	if (!AssetCookerDiscovery::BuildProjectCookPlan(repositoryRoot, projectName, configuration, toolProfile, category, plan, diagnostics))
	{
		return false;
	}

	return AssetCookerDispatcher::DispatchPlan(plan, diagnostics, outOutputs);
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
	result.exitCode = succeeded ? 0 : 1;
	result.diagnostics = diagnostics.ReleaseRecords();
	result.outputs = std::move(outputs);
	return result;
}

bool AssetCookerService::ResolveRepositoryRoot(
    AssetCookerDiagnostics& diagnostics,
    std::filesystem::path& outRepositoryRoot) const
{
	if (!m_configuredRepositoryRoot.empty())
	{
		outRepositoryRoot = std::filesystem::absolute(m_configuredRepositoryRoot).lexically_normal();
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
	return m_configuredProjectName;
}

std::string AssetCookerService::ResolveConfiguration(const char* requestConfiguration) const
{
	if (HasText(requestConfiguration))
	{
		return requestConfiguration;
	}
	if (!m_configuredConfiguration.empty())
	{
		return m_configuredConfiguration;
	}
	return "DevelopmentGame";
}

std::string AssetCookerService::ResolveToolProfile(std::string_view defaultToolProfile) const
{
	if (!m_configuredToolProfile.empty())
	{
		return m_configuredToolProfile;
	}
	return std::string(defaultToolProfile);
}
