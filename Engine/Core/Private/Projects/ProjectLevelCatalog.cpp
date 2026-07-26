#include "PCH.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <fstream>
#include <sstream>

class ProjectLevelCatalogFileOperations final
{
  public:
	enum class Section
	{
		None,
		Level,
		OptionalContentPack
	};

	static constexpr std::string_view CatalogFileName = "Levels.catalog";

	static std::filesystem::path ResolveProjectPath(
	    const std::filesystem::path& projectRoot,
	    std::string_view value);
	static bool SetEntryBool(
	    const std::filesystem::path& projectRoot,
	    std::string_view sectionHeader,
	    std::string_view entryId,
	    std::string_view keyName,
	    bool value,
	    std::string& outErrorMessage);
	static bool IsPathAvailable(const std::filesystem::path& path) noexcept;
};

bool ProjectLevelCatalog::IsOptionalContentPackReady(
    const std::filesystem::path& projectRoot,
    std::string_view packId) const
{
	if (packId.empty())
	{
		return true;
	}

	const auto pack = optionalContentPacks.find(packId);
	if (pack == optionalContentPacks.end() || !pack->second.available)
	{
		return false;
	}

	const std::filesystem::path& configuredRoot = pack->second.rootPath;
	const std::filesystem::path root =
	    configuredRoot.is_relative() ? projectRoot / configuredRoot : configuredRoot;
	return configuredRoot.empty() ||
	       ProjectLevelCatalogFileOperations::IsPathAvailable(root.lexically_normal());
}

bool ProjectLevelCatalog::IsLevelReady(
    const std::filesystem::path& projectRoot,
    const ProjectLevelCatalogEntry& level) const
{
	return !level.id.empty() &&
	       !level.sourcePath.empty() &&
	       ProjectLevelCatalogFileOperations::IsPathAvailable(level.sourcePath) &&
	       IsOptionalContentPackReady(projectRoot, level.optionalContentPackId);
}

bool ProjectLevelCatalogFile::Load(
    const std::filesystem::path& projectRoot,
    ProjectLevelCatalog& outCatalog,
    std::string& outErrorMessage)
{
	outCatalog = {};

	const std::filesystem::path catalogPath =
	    projectRoot / ProjectLevelCatalogFileOperations::CatalogFileName;
	std::ifstream input(catalogPath);
	if (!input.is_open())
	{
		outErrorMessage = "Project level catalog was not found: " + catalogPath.string();
		return false;
	}

	ProjectLevelCatalogFileOperations::Section section =
	    ProjectLevelCatalogFileOperations::Section::None;
	ProjectLevelCatalogEntry* currentLevel = nullptr;
	ProjectOptionalContentPack* currentPack = nullptr;
	for (std::string line; std::getline(input, line);)
	{
		line = Strings::TrimCopy(line);
		if (line.empty() || line.front() == '#' || line.front() == ';')
		{
			continue;
		}

		if (line == "[Level]")
		{
			section = ProjectLevelCatalogFileOperations::Section::Level;
			currentLevel = &outCatalog.levels.emplace_back();
			currentPack = nullptr;
			continue;
		}

		if (line == "[OptionalPack]")
		{
			section = ProjectLevelCatalogFileOperations::Section::OptionalContentPack;
			currentLevel = nullptr;
			currentPack = nullptr;
			continue;
		}

		std::string_view key;
		std::string_view value;
		if (!Strings::TrySplitKeyValue(line, '=', key, value))
		{
			continue;
		}

		if (section == ProjectLevelCatalogFileOperations::Section::Level &&
		    currentLevel != nullptr)
		{
			if (key == "Id")
			{
				currentLevel->id = Strings::UnquoteCopy(value);
			}
			else if (key == "DisplayName")
			{
				currentLevel->displayName = Strings::UnquoteCopy(value);
			}
			else if (key == "Source")
			{
				currentLevel->sourcePath =
				    ProjectLevelCatalogFileOperations::ResolveProjectPath(
				        projectRoot,
				        value);
			}
			else if (key == "OptionalPack")
			{
				currentLevel->optionalContentPackId = Strings::UnquoteCopy(value);
			}
			else if (key == "Default")
			{
				(void) Strings::TryParseBool(value, currentLevel->defaultIncluded);
			}
			else if (key == "Required")
			{
				(void) Strings::TryParseBool(value, currentLevel->required);
			}
			else if (key == "StartupDefault")
			{
				(void) Strings::TryParseBool(value, currentLevel->startupDefault);
			}
			continue;
		}

		if (section != ProjectLevelCatalogFileOperations::Section::OptionalContentPack)
		{
			continue;
		}

		if (key == "Id")
		{
			const std::string id = Strings::UnquoteCopy(value);
			currentPack = &outCatalog.optionalContentPacks[id];
			currentPack->id = id;
		}
		else if (currentPack != nullptr && key == "DisplayName")
		{
			currentPack->displayName = Strings::UnquoteCopy(value);
		}
		else if (currentPack != nullptr && (key == "Root" || key == "Path"))
		{
			currentPack->rootPath =
			    ProjectLevelCatalogFileOperations::ResolveProjectPath(
			        projectRoot,
			        value);
		}
		else if (currentPack != nullptr && key == "Available")
		{
			(void) Strings::TryParseBool(value, currentPack->available);
		}
		else if (currentPack != nullptr && key == "External")
		{
			(void) Strings::TryParseBool(value, currentPack->external);
		}
	}

	outErrorMessage.clear();
	return true;
}

bool ProjectLevelCatalogFile::SetLevelDefaultIncluded(
    const std::filesystem::path& projectRoot,
    std::string_view levelId,
    bool included,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogFileOperations::SetEntryBool(
	    projectRoot,
	    "[Level]",
	    levelId,
	    "Default",
	    included,
	    outErrorMessage);
}

bool ProjectLevelCatalogFile::SetOptionalContentPackAvailable(
    const std::filesystem::path& projectRoot,
    std::string_view packId,
    bool available,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogFileOperations::SetEntryBool(
	    projectRoot,
	    "[OptionalPack]",
	    packId,
	    "Available",
	    available,
	    outErrorMessage);
}

std::filesystem::path ProjectLevelCatalogFileOperations::ResolveProjectPath(
    const std::filesystem::path& projectRoot,
    std::string_view value)
{
	std::filesystem::path path(Strings::UnquoteCopy(value));
	if (path.is_relative())
	{
		path = projectRoot / path;
	}
	return path.lexically_normal();
}

bool ProjectLevelCatalogFileOperations::SetEntryBool(
    const std::filesystem::path& projectRoot,
    std::string_view sectionHeader,
    std::string_view entryId,
    std::string_view keyName,
    bool value,
    std::string& outErrorMessage)
{
	const std::filesystem::path catalogPath =
	    projectRoot / CatalogFileName;
	std::string catalogText;
	if (!Files::TryReadAllText(catalogPath, catalogText, outErrorMessage))
	{
		return false;
	}

	std::vector<std::string> lines;
	std::istringstream input(catalogText);
	for (std::string line; std::getline(input, line);)
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}
		lines.push_back(std::move(line));
	}

	const std::string valueLine =
	    std::string(keyName) + " = " + (value ? "true" : "false");
	bool inRequestedSection = false;
	bool inTargetEntry = false;
	bool targetFound = false;
	bool keyUpdated = false;
	for (std::size_t index = 0; index < lines.size(); ++index)
	{
		const std::string trimmed = Strings::TrimCopy(lines[index]);
		if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';')
		{
			continue;
		}

		if (trimmed.front() == '[' && trimmed.back() == ']')
		{
			if (inTargetEntry)
			{
				lines.insert(
				    lines.begin() + static_cast<std::ptrdiff_t>(index),
				    valueLine);
				targetFound = true;
				keyUpdated = true;
				break;
			}

			inRequestedSection = trimmed == sectionHeader;
			inTargetEntry = false;
			continue;
		}

		if (!inRequestedSection)
		{
			continue;
		}

		std::string_view key;
		std::string_view parsedValue;
		if (!Strings::TrySplitKeyValue(trimmed, '=', key, parsedValue))
		{
			continue;
		}

		if (key == "Id")
		{
			inTargetEntry = Strings::UnquoteCopy(parsedValue) == entryId;
			targetFound = targetFound || inTargetEntry;
		}
		else if (inTargetEntry && key == keyName)
		{
			lines[index] = valueLine;
			keyUpdated = true;
			break;
		}
	}

	if (inTargetEntry && !keyUpdated)
	{
		lines.push_back(valueLine);
	}

	if (!targetFound)
	{
		outErrorMessage =
		    "Project level catalog entry was not found: " +
		    std::string(entryId);
		return false;
	}

	std::string output;
	for (const std::string& line : lines)
	{
		output += line;
		output += '\n';
	}

	return Files::TryWriteAllTextAtomic(
	    catalogPath,
	    output,
	    outErrorMessage);
}

bool ProjectLevelCatalogFileOperations::IsPathAvailable(
    const std::filesystem::path& path) noexcept
{
	std::error_code error;
	return std::filesystem::exists(path, error) && !error;
}
