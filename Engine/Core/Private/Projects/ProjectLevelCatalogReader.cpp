#include "PCH.h"

#include "Projects/ProjectLevelCatalogReader.h"
#include "Projects/ProjectLevelCatalogValidator.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <charconv>
#include <format>
#include <fstream>
#include <istream>
#include <sstream>
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
		ValidateProjectLevelCatalog(reader.m_catalog);
		return std::move(reader.m_catalog);
	}
	catch (const Diagnostics::Error& error)
	{
		throw Diagnostics::Error(std::format("Project level catalog '{}': {}", catalogPath.string(), error.what()));
	}
}

void ProjectLevelCatalogReader::ValidateText(const std::filesystem::path& projectRoot, std::string_view text)
{
	std::istringstream input{std::string(text)};
	ProjectLevelCatalogReader reader(projectRoot);
	reader.ReadCatalog(input);
	ValidateProjectLevelCatalog(reader.m_catalog);
}

ProjectLevelCatalogReader::ProjectLevelCatalogReader(const std::filesystem::path& projectRoot) noexcept :
    m_projectRoot(projectRoot)
{
}

void ProjectLevelCatalogReader::ReadCatalog(std::istream& input)
{
	for (std::string line; std::getline(input, line);)
	{
		++m_lineNumber;
		try
		{
			ParseLine(std::move(line));
		}
		catch (const Diagnostics::Error& error)
		{
			throw Diagnostics::Error(std::format("Line {}: {}", m_lineNumber, error.what()));
		}
	}
	ValidateCurrentSection();
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
	if (line == "[AssetPack]")
	{
		BeginAssetPack();
		return;
	}
	if (line.front() == '[' && line.back() == ']')
	{
		throw Diagnostics::Error(std::format("Unsupported catalog section '{}'.", line));
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
	if (m_section == Section::AssetPack)
	{
		ParseAssetPackField(key, value);
		return;
	}
	throw Diagnostics::Error("Catalog field appears outside a section.");
}

void ProjectLevelCatalogReader::BeginLevel()
{
	ValidateCurrentSection();
	m_section = Section::Level;
	m_currentLevel = &m_catalog.levels.emplace_back();
	m_currentPack = nullptr;
	m_sectionFields.clear();
}

void ProjectLevelCatalogReader::BeginAssetPack()
{
	ValidateCurrentSection();
	m_section = Section::AssetPack;
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
	else if (key == "Description")
	{
		m_currentLevel->description = Strings::UnquoteCopy(value);
	}
	else if (key == "Source")
	{
		m_currentLevel->sourcePath = ResolveProjectPath(value);
	}
	else if (key == "Thumbnail")
	{
		m_currentLevel->thumbnailPath = ResolveProjectPath(value);
	}
	else if (key == "SourcePage")
	{
		m_currentLevel->sourcePageUrl = Strings::UnquoteCopy(value);
	}
	else if (key == "AssetPack")
	{
		m_currentLevel->assetPackId = Strings::UnquoteCopy(value);
	}
	else if (key == "Family")
	{
		m_currentLevel->family = Strings::UnquoteCopy(value);
	}
	else if (key == "VariantKind")
	{
		m_currentLevel->variantKind = Strings::UnquoteCopy(value);
	}
	else if (key == "Selected")
	{
		m_currentLevel->selected = ParseBool(value);
	}
	else
	{
		throw Diagnostics::Error(std::format("Unsupported level catalog field '{}'.", key));
	}
}

void ProjectLevelCatalogReader::ParseAssetPackField(std::string_view key, std::string_view value)
{
	if (key == "Id")
	{
		const std::string id = Strings::UnquoteCopy(value);
		if (id.empty())
		{
			throw Diagnostics::Error("Asset pack identity is empty.");
		}
		if (m_catalog.assetPacks.contains(id))
		{
			throw Diagnostics::Error(std::format("Asset pack identity '{}' is duplicated.", id));
		}
		m_currentPack = &m_catalog.assetPacks.emplace(id, ProjectAssetPack{}).first->second;
		m_currentPack->id = id;
		return;
	}
	if (m_currentPack == nullptr)
	{
		throw Diagnostics::Error("Asset pack field appears before its identity.");
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
	else if (key == "SourceFiles")
	{
		m_currentPack->sourceFilesManifestPath = ResolveProjectPath(value);
	}
	else if (key == "Archive")
	{
		m_currentPack->archiveName = Strings::UnquoteCopy(value);
	}
	else if (key == "ArchiveBytes")
	{
		m_currentPack->archiveBytes = ParseByteCount(value);
	}
	else if (key == "ArchiveSha256")
	{
		m_currentPack->archiveSha256 = Strings::UnquoteCopy(value);
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
		throw Diagnostics::Error(std::format("Unsupported asset pack field '{}'.", key));
	}
}

void ProjectLevelCatalogReader::ValidateCurrentSection() const
{
	const auto requireField = [this](std::string_view field)
	{
		if (!m_sectionFields.contains(std::string(field)))
		{
			throw Diagnostics::Error(std::format("Catalog section is missing required field '{}'.", field));
		}
	};

	if (m_section == Section::Level)
	{
		requireField("Id");
		requireField("Source");
		requireField("Selected");
	}
	else if (m_section == Section::AssetPack)
	{
		requireField("Id");
		requireField("DisplayName");
		requireField("Root");
		requireField("Required");
		requireField("External");
		requireField("DownloadSupported");
		requireField("RuntimeSupported");
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

std::filesystem::path ProjectLevelCatalogReader::ResolveProjectPath(std::string_view value) const
{
	std::filesystem::path path(Strings::UnquoteCopy(value));
	if (path.empty())
	{
		return {};
	}
	if (path.is_relative())
	{
		path = m_projectRoot / path;
	}
	path = path.lexically_normal();
	if (!Paths::IsUnderRoot(path, m_projectRoot))
	{
		throw Diagnostics::Error(std::format("Catalog path must remain below the project root: '{}'.", path.string()));
	}
	return path;
}
