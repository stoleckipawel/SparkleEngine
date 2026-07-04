#include "AssetCookerDiscovery.h"

#include "Core/Public/Strings/StringUtils.h"
#include "SourceSceneImporter.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

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

static bool AssetCookerSceneEntryAllowed(
    const std::filesystem::path& relativePath,
    const std::set<std::string>* allowedSceneIds)
{
	if (allowedSceneIds == nullptr || allowedSceneIds->empty())
	{
		return allowedSceneIds == nullptr;
	}

	std::filesystem::path sceneIdPath = relativePath;
	sceneIdPath.replace_extension();
	return allowedSceneIds->contains(AssetCookerToLower(sceneIdPath.generic_string()));
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
    const std::set<std::string>* allowedSceneIds,
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
		if (!iterator->is_regular_file(statusError) || !SourceSceneImporter::SupportsSourceScenePath(iterator->path()))
		{
			continue;
		}

		std::error_code relativeError;
		const std::filesystem::path relativePath = std::filesystem::relative(iterator->path(), root, relativeError);
		if (relativeError)
		{
			continue;
		}

		if (!AssetCookerSceneEntryAllowed(relativePath, allowedSceneIds))
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

static void AssetCookerCollectLevelSceneIds(const std::filesystem::path& levelPath, std::set<std::string>& outSceneIds)
{
	std::ifstream input(levelPath);
	if (!input.is_open())
	{
		return;
	}

	bool inSceneAssetsSection = false;
	for (std::string line; std::getline(input, line);)
	{
		line = Strings::TrimCopy(line);
		if (line.empty() || line[0] == '#' || line[0] == ';')
		{
			continue;
		}

		if (line.front() == '[' && line.back() == ']')
		{
			inSceneAssetsSection = (line == "[SceneAssets]");
			continue;
		}

		if (!inSceneAssetsSection)
		{
			continue;
		}

		std::string_view key;
		std::string_view value;
		if (Strings::TrySplitKeyValue(line, '=', key, value) && key == "Asset")
		{
			outSceneIds.insert(AssetCookerToLower(Strings::UnquoteCopy(value)));
		}
	}
}

static bool AssetCookerOptionalPackAvailable(
    const std::filesystem::path& projectRoot,
    const std::map<std::string, std::pair<std::filesystem::path, bool>>& optionalPacks,
    std::string_view optionalPackId)
{
	if (optionalPackId.empty())
	{
		return true;
	}

	const auto packIt = optionalPacks.find(std::string(optionalPackId));
	if (packIt == optionalPacks.end() || !packIt->second.second)
	{
		return false;
	}

	if (packIt->second.first.empty())
	{
		return true;
	}

	const std::filesystem::path packPath =
	    packIt->second.first.is_relative() ? (projectRoot / packIt->second.first) : packIt->second.first;
	return AssetCookerPathExists(packPath.lexically_normal());
}

static bool AssetCookerCollectCatalogDefaultSceneIds(
    const std::filesystem::path& projectRoot,
    std::set<std::string>& outSceneIds)
{
	const std::filesystem::path catalogPath = projectRoot / "Levels.catalog";
	std::ifstream input(catalogPath);
	if (!input.is_open())
	{
		return false;
	}

	enum class CatalogSection
	{
		None,
		Level,
		OptionalPack
	};

	struct CatalogLevel final
	{
		std::filesystem::path sourcePath;
		bool defaultIncluded = true;
		std::string optionalPackId;
	};

	std::vector<CatalogLevel> catalogLevels;
	std::map<std::string, std::pair<std::filesystem::path, bool>> optionalPacks;
	CatalogSection section = CatalogSection::None;
	CatalogLevel* currentLevel = nullptr;
	std::string currentOptionalPackId;

	for (std::string line; std::getline(input, line);)
	{
		line = Strings::TrimCopy(line);
		if (line.empty() || line[0] == '#' || line[0] == ';')
		{
			continue;
		}

		if (line == "[Level]")
		{
			section = CatalogSection::Level;
			currentLevel = &catalogLevels.emplace_back();
			currentOptionalPackId.clear();
			continue;
		}
		if (line == "[OptionalPack]")
		{
			section = CatalogSection::OptionalPack;
			currentLevel = nullptr;
			currentOptionalPackId.clear();
			continue;
		}

		std::string_view key;
		std::string_view value;
		if (!Strings::TrySplitKeyValue(line, '=', key, value))
		{
			continue;
		}

		if (section == CatalogSection::Level && currentLevel != nullptr)
		{
			if (key == "Source")
			{
				currentLevel->sourcePath = projectRoot / std::filesystem::path(Strings::UnquoteCopy(value));
			}
			else if (key == "Default")
			{
				bool defaultIncluded = true;
				if (Strings::TryParseBool(value, defaultIncluded))
				{
					currentLevel->defaultIncluded = defaultIncluded;
				}
			}
			else if (key == "OptionalPack")
			{
				currentLevel->optionalPackId = Strings::UnquoteCopy(value);
			}
			continue;
		}

		if (section == CatalogSection::OptionalPack)
		{
			if (key == "Id")
			{
				currentOptionalPackId = Strings::UnquoteCopy(value);
				optionalPacks.try_emplace(currentOptionalPackId, std::filesystem::path(), true);
			}
			else if (!currentOptionalPackId.empty())
			{
				auto& pack = optionalPacks[currentOptionalPackId];
				if (key == "Root" || key == "Path")
				{
					pack.first = std::filesystem::path(Strings::UnquoteCopy(value));
				}
				else if (key == "Available")
				{
					bool available = true;
					if (Strings::TryParseBool(value, available))
					{
						pack.second = available;
					}
				}
			}
		}
	}

	for (const CatalogLevel& level : catalogLevels)
	{
		if (!level.defaultIncluded || level.sourcePath.empty() ||
		    !AssetCookerOptionalPackAvailable(projectRoot, optionalPacks, level.optionalPackId))
		{
			continue;
		}

		AssetCookerCollectLevelSceneIds(level.sourcePath.lexically_normal(), outSceneIds);
	}

	return true;
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
	return configuration == "DebugEditor" || configuration == "DebugGame" || configuration == "DevelopmentEditor" ||
	       configuration == "DevelopmentGame" || configuration == "ShippingEditor" || configuration == "ShippingGame";
}

static std::string AssetCookerResolveToolConfiguration(std::string_view configuration)
{
	std::string toolConfiguration(configuration);
	constexpr std::string_view gameSuffix = "Game";
	if (toolConfiguration.size() >= gameSuffix.size() &&
	    std::string_view(toolConfiguration).substr(toolConfiguration.size() - gameSuffix.size()) == gameSuffix)
	{
		toolConfiguration.resize(toolConfiguration.size() - gameSuffix.size());
		toolConfiguration += "Editor";
	}
	return toolConfiguration;
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
	outPlan.toolConfiguration = AssetCookerResolveToolConfiguration(configuration);
	outPlan.repositoryRoot = repositoryRoot;
	outPlan.projectRoot = repositoryRoot / "Projects" / outPlan.projectName;
	outPlan.cookedRoot = repositoryRoot / "artifacts" / "dev" / "projects" / outPlan.projectName / "cooked";
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
		    nullptr,
		    entriesByKey,
		    outPlan.engineSceneCount,
		    overrideCount);
		std::set<std::string> catalogProjectSceneIds;
		const bool catalogFound = AssetCookerCollectCatalogDefaultSceneIds(outPlan.projectRoot, catalogProjectSceneIds);
		AssetCookerCollectSceneEntries(
		    outPlan.projectRoot / "Assets" / "Meshes",
		    "Project",
		    catalogFound ? &catalogProjectSceneIds : nullptr,
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

	return true;
}
