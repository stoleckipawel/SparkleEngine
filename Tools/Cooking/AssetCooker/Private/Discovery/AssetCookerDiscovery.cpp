#include "AssetCookerDiscovery.h"

#include "CatalogedLevelSceneReader.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "SourceSceneImporter.h"

#include <algorithm>
#include <array>
#include <optional>
#include <system_error>
#include <utility>

bool AssetCookerDiscovery::TryFindRepositoryRoot(
    const std::filesystem::path& startPath,
    std::filesystem::path& outRepositoryRoot)
{
	std::error_code error;
	std::filesystem::path currentPath =
	    Paths::Normalize(startPath.empty() ? std::filesystem::current_path(error) : startPath);
	if (error)
	{
		return false;
	}
	if (!std::filesystem::is_directory(currentPath))
	{
		currentPath = currentPath.parent_path();
	}

	const std::optional<std::filesystem::path> workspaceRoot =
	    Filesystem::FindAncestorWithMarker(currentPath, Filesystem::kWorkspaceMarker);
	if (!workspaceRoot ||
	    !PathExists(*workspaceRoot / "Engine" / std::string(Filesystem::kEngineMarker)) ||
	    !PathExists(*workspaceRoot / "Projects") ||
	    !PathExists(*workspaceRoot / "Tools"))
	{
		return false;
	}

	outRepositoryRoot = *workspaceRoot;
	return true;
}

bool AssetCookerDiscovery::ValidateConfiguration(std::string_view configuration)
{
	return configuration == "DebugEditor" || configuration == "DebugGame" ||
	       configuration == "DevelopmentEditor" || configuration == "DevelopmentGame" ||
	       configuration == "ShippingEditor" || configuration == "ShippingGame";
}

std::vector<std::string> AssetCookerDiscovery::DiscoverProjects(
    const std::filesystem::path& repositoryRoot,
    AssetCookerDiagnostics& diagnostics)
{
	std::vector<std::string> projects;
	const std::filesystem::path projectsRoot = repositoryRoot / "Projects";
	if (!PathExists(projectsRoot))
	{
		diagnostics.AddError(AssetCookerCategory_All, "Projects directory was not found.", projectsRoot);
		return projects;
	}

	std::error_code iteratorError;
	for (const std::filesystem::directory_entry& entry :
	     std::filesystem::directory_iterator(projectsRoot, iteratorError))
	{
		std::error_code statusError;
		const std::string projectName = entry.path().filename().string();
		if (entry.is_directory(statusError) && projectName != "TemplateProject" &&
		    PathExists(entry.path() / std::string(Filesystem::kProjectMarker)))
		{
			projects.push_back(projectName);
		}
	}

	std::sort(projects.begin(), projects.end());
	if (projects.empty())
	{
		diagnostics.AddError(AssetCookerCategory_All, "No runnable projects were found under Projects.");
	}
	return projects;
}

bool AssetCookerDiscovery::BuildProjectCookPlan(
    const std::filesystem::path& repositoryRoot,
    std::string_view projectName,
    std::string_view configuration,
    AssetCookerCategory category,
    AssetCookerProjectCookPlan& outPlan,
    AssetCookerDiagnostics& diagnostics)
{
	outPlan = {};
	outPlan.projectName = std::string(projectName);
	outPlan.configuration = std::string(configuration);
	outPlan.toolConfiguration = ResolveToolConfiguration(configuration);
	outPlan.repositoryRoot = repositoryRoot;
	outPlan.projectRoot = repositoryRoot / "Projects" / outPlan.projectName;
	outPlan.cookedRoot =
	    repositoryRoot / "artifacts" / "dev" / "projects" / outPlan.projectName / "cooked";
	AddPlanSteps(category, outPlan.steps);

	if (!PathExists(outPlan.projectRoot / std::string(Filesystem::kProjectMarker)))
	{
		diagnostics.AddError(
		    AssetCookerCategory_All,
		    "Project marker was not found.",
		    outPlan.projectRoot / std::string(Filesystem::kProjectMarker));
		return false;
	}

	if (!CategoryNeedsScenes(category))
	{
		return true;
	}

	std::vector<std::string> sceneIds;
	if (!CollectSceneIds(
	        outPlan.projectRoot,
	        sceneIds,
	        diagnostics))
	{
		return false;
	}

	for (const std::string& sceneId : sceneIds)
	{
		AssetCookerSceneEntry entry;
		if (!ResolveSceneEntry(outPlan.projectRoot, sceneId, entry, diagnostics))
		{
			return false;
		}
		outPlan.sceneEntries.push_back(std::move(entry));
	}

	if (outPlan.sceneEntries.empty())
	{
		diagnostics.AddError(category, "The default level catalog contains no source scene assets.");
		return false;
	}
	return true;
}

bool AssetCookerDiscovery::PathExists(const std::filesystem::path& path)
{
	std::error_code error;
	return std::filesystem::exists(path, error) && !error;
}

bool AssetCookerDiscovery::CategoryNeedsScenes(AssetCookerCategory category) noexcept
{
	return category == AssetCookerCategory_All ||
	       category == AssetCookerCategory_Textures ||
	       category == AssetCookerCategory_SceneAssets;
}

void AssetCookerDiscovery::AddPlanSteps(
    AssetCookerCategory category,
    std::vector<AssetCookerPlanStep>& outSteps)
{
	outSteps.clear();
	if (category == AssetCookerCategory_All)
	{
		outSteps = {
		    AssetCookerPlanStep::Shaders,
		    AssetCookerPlanStep::Textures,
		    AssetCookerPlanStep::SceneAssets};
		return;
	}

	switch (category)
	{
	case AssetCookerCategory_Shaders:
		outSteps.push_back(AssetCookerPlanStep::Shaders);
		break;
	case AssetCookerCategory_Textures:
		outSteps.push_back(AssetCookerPlanStep::Textures);
		break;
	default:
		outSteps.push_back(AssetCookerPlanStep::SceneAssets);
		break;
	}
}

std::string AssetCookerDiscovery::ResolveToolConfiguration(std::string_view configuration)
{
	std::string toolConfiguration(configuration);
	constexpr std::string_view gameSuffix = "Game";
	if (toolConfiguration.ends_with(gameSuffix))
	{
		toolConfiguration.resize(toolConfiguration.size() - gameSuffix.size());
		toolConfiguration += "Editor";
	}
	return toolConfiguration;
}

bool AssetCookerDiscovery::CollectSceneIds(
    const std::filesystem::path& projectRoot,
    std::vector<std::string>& outSceneIds,
    AssetCookerDiagnostics& diagnostics)
{
	ProjectLevelCatalog catalog;
	std::string errorMessage;
	if (!ProjectLevelCatalogFile::Load(
	        projectRoot,
	        catalog,
	        errorMessage))
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    std::move(errorMessage));
		return false;
	}

	outSceneIds.clear();
	for (const ProjectLevelCatalogEntry& level : catalog.levels)
	{
		if (!level.defaultIncluded && !level.required)
		{
			continue;
		}

		if (!catalog.IsLevelReady(projectRoot, level))
		{
			if (level.required)
			{
				diagnostics.AddError(
				    AssetCookerCategory_SceneAssets,
				    "Required catalog level is unavailable: " + level.id,
				    level.sourcePath);
				return false;
			}
			continue;
		}

		if (!CatalogedLevelSceneReader::AppendSceneIds(
		        level.sourcePath,
		        outSceneIds,
		        errorMessage))
		{
			diagnostics.AddError(
			    AssetCookerCategory_SceneAssets,
			    std::move(errorMessage),
			    level.sourcePath);
			return false;
		}
	}

	std::sort(outSceneIds.begin(), outSceneIds.end());
	outSceneIds.erase(
	    std::unique(outSceneIds.begin(), outSceneIds.end()),
	    outSceneIds.end());
	return true;
}

bool AssetCookerDiscovery::ResolveSceneEntry(
    const std::filesystem::path& projectRoot,
    std::string_view sceneId,
    AssetCookerSceneEntry& outEntry,
    AssetCookerDiagnostics& diagnostics)
{
	const std::filesystem::path meshRoot =
	    Paths::Normalize(projectRoot / "Assets" / "Meshes");
	const std::filesystem::path relativeBase =
	    std::filesystem::path(sceneId).lexically_normal();
	const std::array<std::wstring_view, 3> extensions = {L".gltf", L".glb", L".fbx"};

	if (relativeBase.empty() ||
	    relativeBase.is_absolute() ||
	    relativeBase.generic_string().starts_with(".."))
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    "Catalog scene id must remain under the project mesh root: " +
		        std::string(sceneId));
		return false;
	}

	std::filesystem::path sourcePath;
	const std::filesystem::path exactCandidate =
	    Paths::Normalize(meshRoot / relativeBase);
	if (SourceSceneImporter::SupportsSourceScenePath(relativeBase) &&
	    Paths::IsUnderRoot(exactCandidate, meshRoot) &&
	    PathExists(exactCandidate))
	{
		sourcePath = exactCandidate;
	}
	else
	{
		for (std::wstring_view extension : extensions)
		{
			std::filesystem::path candidate = exactCandidate;
			candidate.replace_extension(extension);
			if (!Paths::IsUnderRoot(candidate, meshRoot) ||
			    !PathExists(candidate))
			{
				continue;
			}
			if (!sourcePath.empty())
			{
				diagnostics.AddError(
				    AssetCookerCategory_SceneAssets,
				    "Catalog scene id resolves to more than one source file: " + std::string(sceneId));
				return false;
			}
			sourcePath = std::move(candidate);
		}
	}

	if (sourcePath.empty())
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    "Catalog scene source was not found: " + std::string(sceneId),
		    meshRoot / relativeBase);
		return false;
	}

	const std::optional<std::filesystem::path> relativePath =
	    Paths::TryMakeRelativeUnderRoot(sourcePath, meshRoot);
	if (!relativePath)
	{
		diagnostics.AddError(
		    AssetCookerCategory_SceneAssets,
		    "Resolved scene source escaped the project mesh root.",
		    sourcePath);
		return false;
	}

	outEntry.relativePath = relativePath->generic_string();
	outEntry.sourcePath = sourcePath;
	return true;
}
