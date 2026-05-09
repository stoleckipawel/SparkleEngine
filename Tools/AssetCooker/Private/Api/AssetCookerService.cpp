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

static AssetCookerCategory AssetCookerNormalizeCategoryForRecook(AssetCookerCategory category)
{
	if (category == AssetCookerCategory_Shader)
	{
		return AssetCookerCategory_Shaders;
	}
	if (category == AssetCookerCategory_Texture)
	{
		return AssetCookerCategory_Textures;
	}
	if (category == AssetCookerCategory_Mesh || category == AssetCookerCategory_Material || category == AssetCookerCategory_Scene)
	{
		return AssetCookerCategory_SceneAssets;
	}
	return category;
}

AssetCookerService::AssetCookerService(const AssetCookerConfig* config)
{
	if (config == nullptr)
	{
		return;
	}

	if (AssetCookerHasText(config->repositoryRoot))
	{
		configuredRepositoryRoot = std::filesystem::path(config->repositoryRoot);
	}
	if (AssetCookerHasText(config->projectName))
	{
		configuredProjectName = config->projectName;
	}
	if (AssetCookerHasText(config->configuration))
	{
		configuredConfiguration = config->configuration;
	}
}

AssetCookerServiceResult AssetCookerService::CookProject(const AssetCookRequest* request)
{
	if (request == nullptr)
	{
		AssetCookerServiceResult result;
		result.succeeded = false;
		result.exitCode = 1;
		result.diagnostics.push_back(
		    {AssetCookerDiagnosticSeverity_Error, AssetCookerCategory_All, "Cook request was null.", std::string()});
		return result;
	}

	return CookCategory(request->projectName, request->configuration, request->category);
}

AssetCookerServiceResult AssetCookerService::RecookAssets(const AssetRecookRequest* request)
{
	AssetCookerDiagnostics diagnostics;
	if (request == nullptr)
	{
		diagnostics.AddError(AssetCookerCategory_All, "Recook request was null.");
		AssetCookerServiceResult result;
		result.succeeded = false;
		result.exitCode = 1;
		result.diagnostics = diagnostics.ReleaseRecords();
		return result;
	}

	const AssetCookerCategory category = ResolveRecookCategory(request, diagnostics);
	AssetCookerServiceResult result = CookCategory(request->projectName, request->configuration, category);
	result.diagnostics.insert(result.diagnostics.begin(), diagnostics.GetRecords().begin(), diagnostics.GetRecords().end());
	return result;
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

AssetCookerServiceResult AssetCookerService::CookCategory(
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
	return "Debug";
}

AssetCookerCategory AssetCookerService::ResolveRecookCategory(
    const AssetRecookRequest* request,
    AssetCookerDiagnostics& diagnostics) const
{
	if (request == nullptr)
	{
		diagnostics.AddError(AssetCookerCategory_All, "Recook request was null.");
		return AssetCookerCategory_All;
	}

	if (request->assets == nullptr || request->assetCount == 0)
	{
		diagnostics.AddWarning(
		    AssetCookerCategory_All,
		    "Recook request contained no selected assets; bridge recook will run the full project cook.");
		return AssetCookerCategory_All;
	}

	AssetCookerCategory category = AssetCookerNormalizeCategoryForRecook(request->assets[0].category);
	for (std::uint32_t index = 1; index < request->assetCount; ++index)
	{
		const AssetCookerCategory currentCategory = AssetCookerNormalizeCategoryForRecook(request->assets[index].category);
		if (currentCategory != category)
		{
			diagnostics.AddWarning(
			    AssetCookerCategory_All,
			    "Selected recook spans multiple current bridge categories; running the full project cook.");
			return AssetCookerCategory_All;
		}
	}

	diagnostics.AddInfo(
	    category,
	    "Selected recook is represented through the AssetCooker API; Phase 2 dispatches the matching current category flow.");
	return category;
}
