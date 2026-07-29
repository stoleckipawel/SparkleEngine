#include "PCH.h"

#include "Projects/ProjectLevelCatalogReader.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <format>
#include <fstream>
#include <istream>
#include <utility>

ProjectLevelCatalog ProjectLevelCatalogReader::Read(const std::filesystem::path& projectRoot)
{
	const std::filesystem::path catalogPath = projectRoot / "Levels.catalog";
	std::ifstream input(catalogPath);
	if (!input.is_open())
		throw Diagnostics::Error("Project level catalog was not found: " + catalogPath.string());

	try
	{
		ProjectLevelCatalogReader reader(projectRoot);
		reader.ReadCatalog(input);
		if (input.bad())
			throw Diagnostics::Error("Project level catalog could not be read.");
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
		ParseLine(std::move(line));
}

void ProjectLevelCatalogReader::ParseLine(std::string line)
{
	line = Strings::TrimCopy(line);
	if (line.empty() || line.front() == '#' || line.front() == ';')
		return;

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
		throw Diagnostics::Error("Malformed catalog field.");
	if (key.empty())
		throw Diagnostics::Error("Catalog field name is empty.");
	if (!m_sectionFields.emplace(key).second)
		throw Diagnostics::Error(std::format("Catalog section repeats field '{}'.", key));

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
		m_currentLevel->id = Strings::UnquoteCopy(value);
	else if (key == "DisplayName")
		m_currentLevel->displayName = Strings::UnquoteCopy(value);
	else if (key == "Source")
		m_currentLevel->sourcePath = ResolveProjectPath(value);
	else if (key == "OptionalPack")
		m_currentLevel->optionalContentPackId = Strings::UnquoteCopy(value);
	else if (key == "Default")
		m_currentLevel->defaultIncluded = ParseBool(value);
	else if (key == "StartupDefault")
		m_currentLevel->startupDefault = ParseBool(value);
	else
		throw Diagnostics::Error(std::format("Unsupported level catalog field '{}'.", key));
}

void ProjectLevelCatalogReader::ParseOptionalContentPackField(std::string_view key, std::string_view value)
{
	if (key == "Id")
	{
		const std::string id = Strings::UnquoteCopy(value);
		if (id.empty())
			throw Diagnostics::Error("Optional content pack identity is empty.");
		if (m_catalog.optionalContentPacks.contains(id))
			throw Diagnostics::Error(std::format("Optional content pack identity '{}' is duplicated.", id));
		m_currentPack = &m_catalog.optionalContentPacks.emplace(id, ProjectOptionalContentPack{}).first->second;
		m_currentPack->id = id;
		return;
	}
	if (m_currentPack == nullptr)
		throw Diagnostics::Error("Optional content pack field appears before its identity.");
	if (key == "DisplayName")
		m_currentPack->displayName = Strings::UnquoteCopy(value);
	else if (key == "Root")
		m_currentPack->rootPath = ResolveProjectPath(value);
	else if (key == "Available")
		m_currentPack->available = ParseBool(value);
	else if (key == "External")
		m_currentPack->external = ParseBool(value);
	else
		throw Diagnostics::Error(std::format("Unsupported optional content pack field '{}'.", key));
}

bool ProjectLevelCatalogReader::ParseBool(std::string_view value) const
{
	bool parsed = false;
	if (!Strings::TryParseBool(value, parsed))
		throw Diagnostics::Error(std::format("Invalid catalog boolean value '{}'.", value));
	return parsed;
}

void ProjectLevelCatalogReader::ValidateCatalog() const
{
	if (m_catalog.levels.empty())
		throw Diagnostics::Error("Catalog contains no levels.");

	std::unordered_set<std::string_view> levelIds;
	std::size_t startupDefaultCount = 0;
	for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
	{
		if (level.id.empty())
			throw Diagnostics::Error("Catalog contains a level with no identity.");
		if (level.sourcePath.empty())
			throw Diagnostics::Error(std::format("Catalog level '{}' has no source path.", level.id));
		if (!levelIds.insert(level.id).second)
			throw Diagnostics::Error(std::format("Catalog level identity '{}' is duplicated.", level.id));
		if (!level.optionalContentPackId.empty() && !m_catalog.optionalContentPacks.contains(level.optionalContentPackId))
			throw Diagnostics::Error(
			    std::format("Catalog level '{}' references unknown content pack '{}'.", level.id, level.optionalContentPackId));
		startupDefaultCount += level.startupDefault ? 1u : 0u;
	}

	for (const auto& [packId, pack] : m_catalog.optionalContentPacks)
	{
		if (packId.empty() || pack.id != packId)
			throw Diagnostics::Error("Catalog contains an invalid optional content pack identity.");
	}
	if (startupDefaultCount != 1u)
		throw Diagnostics::Error("Catalog must identify exactly one startup default level.");
}

std::filesystem::path ProjectLevelCatalogReader::ResolveProjectPath(std::string_view value) const
{
	std::filesystem::path path(Strings::UnquoteCopy(value));
	if (path.is_relative())
		path = m_projectRoot / path;
	return path.lexically_normal();
}
