#include "AssetCookerDiscovery.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <system_error>
#include <utility>

static bool AssetCookerPathExists(const std::filesystem::path& path)
{
	std::error_code errorCode;
	return std::filesystem::exists(path, errorCode);
}

static std::string AssetCookerToLower(std::string value)
{
	std::transform(
	    value.begin(),
	    value.end(),
	    value.begin(),
	    [](unsigned char character) -> char
	    {
		    return static_cast<char>(std::tolower(character));
	    });
	return value;
}

static bool AssetCookerIsSupportedSceneExtension(const std::filesystem::path& path)
{
	const std::string extension = AssetCookerToLower(path.extension().string());
	return extension == ".gltf" || extension == ".glb" || extension == ".fbx";
}

static bool AssetCookerCategoryNeedsScenes(AssetCookerCategory category)
{
	return category == AssetCookerCategory_All || category == AssetCookerCategory_Textures ||
	       category == AssetCookerCategory_SceneAssets || category == AssetCookerCategory_Texture ||
	       category == AssetCookerCategory_Mesh || category == AssetCookerCategory_Material ||
	       category == AssetCookerCategory_Scene;
}

static void AssetCookerAddPlanSteps(AssetCookerCategory category, std::vector<AssetCookerPlanStep>& outSteps)
{
	outSteps.clear();
	if (category == AssetCookerCategory_All)
	{
		outSteps.push_back(AssetCookerPlanStep::Shaders);
		outSteps.push_back(AssetCookerPlanStep::Textures);
		outSteps.push_back(AssetCookerPlanStep::SceneAssets);
		return;
	}

	if (category == AssetCookerCategory_Shaders || category == AssetCookerCategory_Shader)
	{
		outSteps.push_back(AssetCookerPlanStep::Shaders);
		return;
	}

	if (category == AssetCookerCategory_Textures || category == AssetCookerCategory_Texture)
	{
		outSteps.push_back(AssetCookerPlanStep::Textures);
		return;
	}

	outSteps.push_back(AssetCookerPlanStep::SceneAssets);
}

static void AssetCookerCollectSceneEntries(
    const std::filesystem::path& root,
    std::string_view origin,
    std::map<std::string, AssetCookerSceneEntry>& entriesByKey,
    int& sourceCount,
    int& overrideCount)
{
	if (!AssetCookerPathExists(root))
	{
		return;
	}

	std::error_code iteratorError;
	std::filesystem::recursive_directory_iterator iterator(
	    root,
	    std::filesystem::directory_options::skip_permission_denied,
	    iteratorError);
	std::filesystem::recursive_directory_iterator endIterator;
	for (; iterator != endIterator; iterator.increment(iteratorError))
	{
		if (iteratorError)
		{
			iteratorError.clear();
			continue;
		}

		std::error_code statusError;
		if (!iterator->is_regular_file(statusError) || !AssetCookerIsSupportedSceneExtension(iterator->path()))
		{
			continue;
		}

		std::error_code relativeError;
		const std::filesystem::path relativePath = std::filesystem::relative(iterator->path(), root, relativeError);
		if (relativeError)
		{
			continue;
		}

		++sourceCount;
		AssetCookerSceneEntry entry;
		entry.origin = std::string(origin);
		entry.relativePath = relativePath.generic_string();
		entry.sourcePath = std::filesystem::absolute(iterator->path()).lexically_normal();

		const std::string key = AssetCookerToLower(entry.relativePath);
		const auto existingEntry = entriesByKey.find(key);
		if (existingEntry != entriesByKey.end() && existingEntry->second.origin == "Engine" && entry.origin == "Project")
		{
			++overrideCount;
		}

		entriesByKey[key] = std::move(entry);
	}
}

bool AssetCookerDiscovery::TryFindRepositoryRoot(
    const std::filesystem::path& startPath,
    std::filesystem::path& outRepositoryRoot)
{
	std::filesystem::path currentPath = std::filesystem::absolute(startPath).lexically_normal();
	if (!std::filesystem::is_directory(currentPath))
	{
		currentPath = currentPath.parent_path();
	}

	while (!currentPath.empty())
	{
		if (AssetCookerPathExists(currentPath / "CMakeLists.txt") && AssetCookerPathExists(currentPath / "Engine") &&
		    AssetCookerPathExists(currentPath / "Projects") && AssetCookerPathExists(currentPath / "Tools"))
		{
			outRepositoryRoot = currentPath;
			return true;
		}

		const std::filesystem::path parentPath = currentPath.parent_path();
		if (parentPath == currentPath)
		{
			break;
		}
		currentPath = parentPath;
	}

	return false;
}

bool AssetCookerDiscovery::ValidateConfiguration(std::string_view configuration)
{
	return configuration == "Debug" || configuration == "Release" || configuration == "RelWithDebInfo";
}

std::vector<std::string> AssetCookerDiscovery::DiscoverProjects(
    const std::filesystem::path& repositoryRoot,
    AssetCookerDiagnostics& diagnostics)
{
	std::vector<std::string> projects;
	const std::filesystem::path projectsRoot = repositoryRoot / "Projects";
	if (!AssetCookerPathExists(projectsRoot))
	{
		diagnostics.AddError(AssetCookerCategory_All, "Projects directory was not found.", projectsRoot);
		return projects;
	}

	std::error_code iteratorError;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(projectsRoot, iteratorError))
	{
		std::error_code statusError;
		if (!entry.is_directory(statusError))
		{
			continue;
		}

		const std::string projectName = entry.path().filename().string();
		if (projectName == "TemplateProject")
		{
			continue;
		}

		if (AssetCookerPathExists(entry.path() / ".sparkle-project"))
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
	outPlan = AssetCookerProjectCookPlan();
	outPlan.projectName = std::string(projectName);
	outPlan.configuration = std::string(configuration);
	outPlan.repositoryRoot = repositoryRoot;
	outPlan.projectRoot = repositoryRoot / "Projects" / outPlan.projectName;
	outPlan.cookedRoot = repositoryRoot / "build" / "Cooked" / outPlan.projectName;
	outPlan.planPath = repositoryRoot / "build" / "Cook" / "Plans" / (outPlan.projectName + ".assetcookplan.txt");
	AssetCookerAddPlanSteps(category, outPlan.steps);

	if (!AssetCookerPathExists(outPlan.projectRoot / ".sparkle-project"))
	{
		diagnostics.AddError(AssetCookerCategory_All, "Project marker was not found.", outPlan.projectRoot / ".sparkle-project");
		return false;
	}

	if (AssetCookerCategoryNeedsScenes(category))
	{
		std::map<std::string, AssetCookerSceneEntry> entriesByKey;
		int overrideCount = 0;
		AssetCookerCollectSceneEntries(
		    repositoryRoot / "Engine" / "Assets" / "Meshes",
		    "Engine",
		    entriesByKey,
		    outPlan.engineSceneCount,
		    overrideCount);
		AssetCookerCollectSceneEntries(
		    outPlan.projectRoot / "Assets" / "Meshes",
		    "Project",
		    entriesByKey,
		    outPlan.projectSceneCount,
		    overrideCount);
		outPlan.overriddenEngineSceneCount = overrideCount;

		for (const auto& entryByKey : entriesByKey)
		{
			outPlan.sceneEntries.push_back(entryByKey.second);
		}

		if (outPlan.sceneEntries.empty())
		{
			diagnostics.AddError(
			    category,
			    "No supported source scenes were found under engine or project mesh roots.");
			return false;
		}
	}

	return WritePlanSummary(outPlan, diagnostics);
}

bool AssetCookerDiscovery::WritePlanSummary(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics)
{
	std::error_code createError;
	std::filesystem::create_directories(plan.planPath.parent_path(), createError);
	if (createError)
	{
		diagnostics.AddError(AssetCookerCategory_All, "Failed to create cook plan directory.", plan.planPath.parent_path());
		return false;
	}

	std::ofstream output(plan.planPath, std::ios::binary);
	if (!output.is_open())
	{
		diagnostics.AddError(AssetCookerCategory_All, "Failed to write cook plan summary.", plan.planPath);
		return false;
	}

	output << "schema=asset-cooker-plan-v1\n";
	output << "project=" << plan.projectName << "\n";
	output << "configuration=" << plan.configuration << "\n";
	output << "engineSceneCount=" << plan.engineSceneCount << "\n";
	output << "projectSceneCount=" << plan.projectSceneCount << "\n";
	output << "overriddenEngineSceneCount=" << plan.overriddenEngineSceneCount << "\n";
	output << "sceneCount=" << plan.sceneEntries.size() << "\n";
	for (const AssetCookerSceneEntry& entry : plan.sceneEntries)
	{
		output << "scene=" << entry.origin << "|" << entry.relativePath << "|" << entry.sourcePath.string() << "\n";
	}

	diagnostics.AddInfo(AssetCookerCategory_All, "ProjectCookPlan summary written to " + plan.planPath.string() + ".");
	return true;
}
