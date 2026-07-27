#include "PCH.h"

#include "Projects/ProjectLevelCatalogReader.h"

#include "Core/Public/Strings/StringUtils.h"

#include <fstream>
#include <istream>
#include <utility>

bool ProjectLevelCatalogReader::Read(
    const std::filesystem::path& projectRoot,
    ProjectLevelCatalog& outCatalog,
    std::string& outErrorMessage)
{
	outCatalog = {};

	const std::filesystem::path catalogPath = projectRoot / "Levels.catalog";
	std::ifstream input(catalogPath);
	if (!input.is_open())
	{
		outErrorMessage = "Project level catalog was not found: " + catalogPath.string();
		return false;
	}

	ProjectLevelCatalogReader reader(projectRoot, outCatalog);
	reader.ReadCatalog(input);

	outErrorMessage.clear();
	return true;
}

ProjectLevelCatalogReader::ProjectLevelCatalogReader(
    const std::filesystem::path& projectRoot,
    ProjectLevelCatalog& catalog) noexcept :
    m_projectRoot(projectRoot),
    m_catalog(catalog)
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
		return;
	}

	if (m_section == Section::Level && m_currentLevel != nullptr)
	{
		ParseLevelField(key, value);
		return;
	}

	if (m_section == Section::OptionalContentPack)
	{
		ParseOptionalContentPackField(key, value);
	}
}

void ProjectLevelCatalogReader::BeginLevel()
{
	m_section = Section::Level;
	m_currentLevel = &m_catalog.levels.emplace_back();
	m_currentPack = nullptr;
}

void ProjectLevelCatalogReader::BeginOptionalContentPack() noexcept
{
	m_section = Section::OptionalContentPack;
	m_currentLevel = nullptr;
	m_currentPack = nullptr;
}

void ProjectLevelCatalogReader::ParseLevelField(
    std::string_view key,
    std::string_view value)
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
	else if (key == "Default")
	{
		(void) Strings::TryParseBool(value, m_currentLevel->defaultIncluded);
	}
	else if (key == "Required")
	{
		(void) Strings::TryParseBool(value, m_currentLevel->required);
	}
	else if (key == "StartupDefault")
	{
		(void) Strings::TryParseBool(value, m_currentLevel->startupDefault);
	}
}

void ProjectLevelCatalogReader::ParseOptionalContentPackField(
    std::string_view key,
    std::string_view value)
{
	if (key == "Id")
	{
		const std::string id = Strings::UnquoteCopy(value);
		m_currentPack = &m_catalog.optionalContentPacks[id];
		m_currentPack->id = id;
	}
	else if (m_currentPack != nullptr && key == "DisplayName")
	{
		m_currentPack->displayName = Strings::UnquoteCopy(value);
	}
	else if (m_currentPack != nullptr && (key == "Root" || key == "Path"))
	{
		m_currentPack->rootPath = ResolveProjectPath(value);
	}
	else if (m_currentPack != nullptr && key == "Available")
	{
		(void) Strings::TryParseBool(value, m_currentPack->available);
	}
	else if (m_currentPack != nullptr && key == "External")
	{
		(void) Strings::TryParseBool(value, m_currentPack->external);
	}
}

std::filesystem::path ProjectLevelCatalogReader::ResolveProjectPath(
    std::string_view value) const
{
	std::filesystem::path path(Strings::UnquoteCopy(value));
	if (path.is_relative())
	{
		path = m_projectRoot / path;
	}

	return path.lexically_normal();
}
