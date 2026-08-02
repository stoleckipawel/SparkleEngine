#include "PCH.h"

#include "Projects/ProjectLevelCatalogReader.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <format>
#include <fstream>
#include <istream>
#include <system_error>
#include <utility>

ProjectLevelCatalog ProjectLevelCatalogReader::Read(const std::filesystem::path& projectRoot)
{
	const std::filesystem::path catalogPath = projectRoot / "Levels.catalog";
	std::ifstream input(catalogPath);
	if (!input.is_open())
	{
		throw Diagnostics::Error("Project level catalog was not found: " + catalogPath.string());
	}

	try
	{
		ProjectLevelCatalogReader reader(projectRoot);
		reader.ReadCatalog(input);
		if (input.bad())
		{
			throw Diagnostics::Error("Project level catalog could not be read.");
		}
		reader.ValidateCatalog();
		return std::move(reader.m_catalog);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(std::format("Project level catalog '{}': {}", catalogPath.string(), error.what()));
	}
}

ProjectLevelCatalogReader::ProjectLevelCatalogReader(const std::filesystem::path& projectRoot) noexcept :
    m_projectRoot(projectRoot)
{
}

void ProjectLevelCatalogReader::ReadCatalog(std::istream& input)
{
	for (std::string line; std::getline(input, line);)
	{
		ParseLine(std::move(line));
	}
}

void ProjectLevelCatalogReader::ParseLine(std::string line)
{
	line = Strings::TrimCopy(line);
	if (line.empty() || line.front() == '#' || line.front() == ';')
	{
		return;
	}

	if (line == "[Level]")
	{
		BeginLevel();
		return;
	}
	if (line == "[OptionalPack]")
	{
		BeginOptionalContentPack();
		return;
	}

	std::string_view key;
	std::string_view value;
	if (!Strings::TrySplitKeyValue(line, '=', key, value))
	{
		throw Diagnostics::Error("Malformed catalog field.");
	}
	if (key.empty())
	{
		throw Diagnostics::Error("Catalog field name is empty.");
	}
	if (!m_sectionFields.emplace(key).second)
	{
		throw Diagnostics::Error(std::format("Catalog section repeats field '{}'.", key));
	}

	if (m_section == Section::Level && m_currentLevel != nullptr)
	{
		ParseLevelField(key, value);
		return;
	}
	if (m_section == Section::OptionalContentPack)
	{
		ParseOptionalContentPackField(key, value);
		return;
	}
	throw Diagnostics::Error("Catalog field appears outside a section.");
}

void ProjectLevelCatalogReader::BeginLevel()
{
	m_section = Section::Level;
	m_currentLevel = &m_catalog.levels.emplace_back();
	m_currentPack = nullptr;
	m_sectionFields.clear();
}

void ProjectLevelCatalogReader::BeginOptionalContentPack() noexcept
{
	m_section = Section::OptionalContentPack;
	m_currentLevel = nullptr;
	m_currentPack = nullptr;
	m_sectionFields.clear();
}

void ProjectLevelCatalogReader::ParseLevelField(std::string_view key, std::string_view value)
{
	if (key == "Id")
	{
		m_currentLevel->id = Strings::UnquoteCopy(value);
	}
	else if (key == "DisplayName")
	{
		m_currentLevel->displayName = Strings::UnquoteCopy(value);
	}
	else if (key == "Source")
	{
		m_currentLevel->sourcePath = ResolveProjectPath(value);
	}
	else if (key == "OptionalPack")
	{
		m_currentLevel->optionalContentPackId = Strings::UnquoteCopy(value);
	}
	else if (key == "Family")
	{
		m_currentLevel->family = Strings::UnquoteCopy(value);
	}
	else if (key == "VariantKind")
	{
		m_currentLevel->variantKind = Strings::UnquoteCopy(value);
	}
	else if (key == "Default")
	{
		m_currentLevel->defaultIncluded = ParseBool(value);
	}
	else if (key == "StartupDefault")
	{
		m_currentLevel->startupDefault = ParseBool(value);
	}
	else
	{
		throw Diagnostics::Error(std::format("Unsupported level catalog field '{}'.", key));
	}
}

void ProjectLevelCatalogReader::ParseOptionalContentPackField(std::string_view key, std::string_view value)
{
	if (key == "Id")
	{
		const std::string id = Strings::UnquoteCopy(value);
		if (id.empty())
		{
			throw Diagnostics::Error("Optional content pack identity is empty.");
		}
		if (m_catalog.optionalContentPacks.contains(id))
		{
			throw Diagnostics::Error(std::format("Optional content pack identity '{}' is duplicated.", id));
		}
		m_currentPack = &m_catalog.optionalContentPacks.emplace(id, ProjectOptionalContentPack{}).first->second;
		m_currentPack->id = id;
		return;
	}
	if (m_currentPack == nullptr)
	{
		throw Diagnostics::Error("Optional content pack field appears before its identity.");
	}
	if (key == "DisplayName")
	{
		m_currentPack->displayName = Strings::UnquoteCopy(value);
	}
	else if (key == "Root")
	{
		m_currentPack->rootPath = ResolveProjectPath(value);
	}
	else if (key == "ExtractRoot")
	{
		m_currentPack->extractionPath = ResolveProjectPath(value);
	}
	else if (key == "Required")
	{
		m_currentPack->requiredRelativePath = std::filesystem::path(Strings::UnquoteCopy(value)).lexically_normal();
	}
	else if (key == "Parent")
	{
		m_currentPack->parentPackId = Strings::UnquoteCopy(value);
	}
	else if (key == "Kind")
	{
		m_currentPack->contentKind = Strings::UnquoteCopy(value);
	}
	else if (key == "SourceUrl")
	{
		m_currentPack->sourceUrl = Strings::UnquoteCopy(value);
	}
	else if (key == "SourcePage")
	{
		m_currentPack->sourcePageUrl = Strings::UnquoteCopy(value);
	}
	else if (key == "Archive")
	{
		m_currentPack->archiveName = Strings::UnquoteCopy(value);
	}
	else if (key == "ArchiveBytes")
	{
		m_currentPack->archiveBytes = ParseByteCount(value);
	}
	else if (key == "Version")
	{
		m_currentPack->version = Strings::UnquoteCopy(value);
	}
	else if (key == "License")
	{
		m_currentPack->license = Strings::UnquoteCopy(value);
	}
	else if (key == "RuntimeBlocker")
	{
		m_currentPack->runtimeBlocker = Strings::UnquoteCopy(value);
	}
	else if (key == "DownloadBlocker")
	{
		m_currentPack->downloadBlocker = Strings::UnquoteCopy(value);
	}
	else if (key == "Available")
	{
		m_currentPack->available = ParseBool(value);
	}
	else if (key == "External")
	{
		m_currentPack->external = ParseBool(value);
	}
	else if (key == "DownloadSupported")
	{
		m_currentPack->downloadSupported = ParseBool(value);
	}
	else if (key == "RuntimeSupported")
	{
		m_currentPack->runtimeSupported = ParseBool(value);
	}
	else
	{
		throw Diagnostics::Error(std::format("Unsupported optional content pack field '{}'.", key));
	}
}

bool ProjectLevelCatalogReader::ParseBool(std::string_view value) const
{
	bool parsed = false;
	if (!Strings::TryParseBool(value, parsed))
	{
		throw Diagnostics::Error(std::format("Invalid catalog boolean value '{}'.", value));
	}
	return parsed;
}

std::uintmax_t ProjectLevelCatalogReader::ParseByteCount(std::string_view value) const
{
	const std::string text = Strings::UnquoteCopy(value);
	std::uintmax_t parsed = 0;
	const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), parsed);
	if (error != std::errc() || end != text.data() + text.size())
	{
		throw Diagnostics::Error(std::format("Invalid catalog byte count '{}'.", value));
	}
	return parsed;
}

void ProjectLevelCatalogReader::ValidateCatalog() const
{
	if (m_catalog.levels.empty())
	{
		throw Diagnostics::Error("Catalog contains no levels.");
	}

	std::unordered_set<std::string_view> levelIds;
	std::size_t startupDefaultCount = 0;
	for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
	{
		if (level.id.empty())
		{
			throw Diagnostics::Error("Catalog contains a level with no identity.");
		}
		if (level.sourcePath.empty())
		{
			throw Diagnostics::Error(std::format("Catalog level '{}' has no source path.", level.id));
		}
		if (!levelIds.insert(level.id).second)
		{
			throw Diagnostics::Error(std::format("Catalog level identity '{}' is duplicated.", level.id));
		}
		if (!level.optionalContentPackId.empty() && !m_catalog.optionalContentPacks.contains(level.optionalContentPackId))
		{
			throw Diagnostics::Error(
			    std::format("Catalog level '{}' references unknown content pack '{}'.", level.id, level.optionalContentPackId));
		}
		startupDefaultCount += level.startupDefault ? 1u : 0u;
	}

	for (const auto& [packId, pack] : m_catalog.optionalContentPacks)
	{
		if (packId.empty() || pack.id != packId)
		{
			throw Diagnostics::Error("Catalog contains an invalid optional content pack identity.");
		}
		if (!std::all_of(
		        pack.id.begin(),
		        pack.id.end(),
		        [](unsigned char character) { return std::isalnum(character) != 0 || character == '-' || character == '_'; }))
		{
			throw Diagnostics::Error(std::format("Optional content pack '{}' has an unsafe identity.", pack.id));
		}
		if (!pack.parentPackId.empty() && !m_catalog.optionalContentPacks.contains(pack.parentPackId))
		{
			throw Diagnostics::Error(std::format("Optional content pack '{}' references unknown parent '{}'.", pack.id, pack.parentPackId));
		}
		if (pack.parentPackId == pack.id)
		{
			throw Diagnostics::Error(std::format("Optional content pack '{}' cannot be its own parent.", pack.id));
		}
		if (!pack.requiredRelativePath.empty()
		    && (pack.requiredRelativePath.is_absolute() || pack.requiredRelativePath.generic_string().starts_with("..")))
		{
			throw Diagnostics::Error(std::format("Optional content pack '{}' has an unsafe required path.", pack.id));
		}
		if (pack.downloadSupported
		    && (pack.sourceUrl.empty() || pack.archiveName.empty() || pack.archiveBytes == 0 || pack.extractionPath.empty()
		        || pack.rootPath.empty() || pack.requiredRelativePath.empty()))
		{
			throw Diagnostics::Error(std::format("Downloadable optional content pack '{}' has incomplete acquisition metadata.", pack.id));
		}
		const std::filesystem::path rootRelativeToExtraction = pack.rootPath.lexically_relative(pack.extractionPath);
		if (pack.downloadSupported && (rootRelativeToExtraction.empty() || rootRelativeToExtraction.generic_string().starts_with("..")))
		{
			throw Diagnostics::Error(std::format("Optional content pack '{}' root must remain below its extraction root.", pack.id));
		}
		if (pack.downloadSupported && std::filesystem::path(pack.archiveName).filename() != pack.archiveName)
		{
			throw Diagnostics::Error(std::format("Optional content pack '{}' archive name must not contain a path.", pack.id));
		}
		if (!pack.downloadSupported && pack.available && !pack.downloadBlocker.empty())
		{
			throw Diagnostics::Error(std::format("Unavailable optional content pack '{}' cannot be selected for download.", pack.id));
		}

		std::unordered_set<std::string_view> ancestors;
		const ProjectOptionalContentPack* ancestor = &pack;
		while (!ancestor->parentPackId.empty())
		{
			if (!ancestors.insert(ancestor->id).second)
			{
				throw Diagnostics::Error(std::format("Optional content pack '{}' has a cyclic parent chain.", pack.id));
			}
			ancestor = &m_catalog.optionalContentPacks.at(ancestor->parentPackId);
		}
	}
	if (startupDefaultCount != 1u)
	{
		throw Diagnostics::Error("Catalog must identify exactly one startup default level.");
	}
}

std::filesystem::path ProjectLevelCatalogReader::ResolveProjectPath(std::string_view value) const
{
	std::filesystem::path path(Strings::UnquoteCopy(value));
	if (path.is_relative())
	{
		path = m_projectRoot / path;
	}
	return path.lexically_normal();
}
