#include "PCH.h"

#include "Projects/ProjectLevelCatalogReader.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
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
		reader.ValidateCatalog();
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
	reader.ValidateCatalog();
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

void ProjectLevelCatalogReader::ValidateCatalog() const
{
	if (m_catalog.levels.empty())
	{
		throw Diagnostics::Error("Catalog contains no levels.");
	}

	std::unordered_set<std::string_view> levelIds;
	for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
	{
		if (!IsSafeIdentifier(level.id))
		{
			throw Diagnostics::Error(std::format("Catalog level '{}' has an unsafe identity.", level.id));
		}
		if (level.sourcePath.empty())
		{
			throw Diagnostics::Error(std::format("Catalog level '{}' has no source path.", level.id));
		}
		if (!levelIds.insert(level.id).second)
		{
			throw Diagnostics::Error(std::format("Catalog level identity '{}' is duplicated.", level.id));
		}
		if (!level.assetPackId.empty() && !m_catalog.assetPacks.contains(level.assetPackId))
		{
			throw Diagnostics::Error(std::format("Catalog level '{}' references unknown asset pack '{}'.", level.id, level.assetPackId));
		}
	}

	std::unordered_set<std::string_view> archiveNames;
	std::vector<const ProjectAssetPack*> downloadablePacks;
	for (const auto& [packId, pack] : m_catalog.assetPacks)
	{
		if (packId.empty() || pack.id != packId)
		{
			throw Diagnostics::Error("Catalog contains an invalid asset pack identity.");
		}
		if (!IsSafeIdentifier(pack.id))
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' has an unsafe identity.", pack.id));
		}
		if (pack.displayName.empty())
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' has no display name.", pack.id));
		}
		if (!pack.parentPackId.empty() && !m_catalog.assetPacks.contains(pack.parentPackId))
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' references unknown parent '{}'.", pack.id, pack.parentPackId));
		}
		if (pack.parentPackId == pack.id)
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' cannot be its own parent.", pack.id));
		}
		if (pack.rootPath.empty())
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' has no content root.", pack.id));
		}
		if (pack.requiredRelativePath.empty() || pack.requiredRelativePath == "." || pack.requiredRelativePath.has_root_name()
		    || pack.requiredRelativePath.has_root_directory() || pack.requiredRelativePath.is_absolute()
		    || pack.requiredRelativePath.generic_string().starts_with(".."))
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' has an unsafe required path.", pack.id));
		}
		if (pack.downloadSupported && !pack.external)
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack '{}' must be declared external.", pack.id));
		}
		if (pack.downloadSupported
		    && (pack.sourceUrl.empty() || pack.archiveName.empty() || pack.archiveBytes == 0 || pack.archiveSha256.empty()
		        || pack.extractionPath.empty()))
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack '{}' has incomplete acquisition metadata.", pack.id));
		}
		if (pack.downloadSupported && !pack.sourceUrl.starts_with("https://"))
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack '{}' must use an HTTPS source URL.", pack.id));
		}
		if (pack.downloadSupported && !IsSha256(pack.archiveSha256))
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack '{}' has an invalid SHA-256 digest.", pack.id));
		}
		if (!pack.runtimeSupported && pack.runtimeBlocker.empty())
		{
			throw Diagnostics::Error(std::format("Runtime-unsupported asset pack '{}' must declare a runtime blocker.", pack.id));
		}
		if (pack.runtimeSupported && !pack.runtimeBlocker.empty())
		{
			throw Diagnostics::Error(std::format("Runtime-supported asset pack '{}' declares a contradictory blocker.", pack.id));
		}
		if (pack.external && !pack.downloadSupported && pack.downloadBlocker.empty())
		{
			throw Diagnostics::Error(
			    std::format("External asset pack '{}' without acquisition support must declare its blocker.", pack.id));
		}
		if (pack.downloadSupported && !pack.downloadBlocker.empty())
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack '{}' declares a contradictory blocker.", pack.id));
		}
		if (pack.external && (pack.sourcePageUrl.empty() || pack.version.empty() || pack.license.empty()))
		{
			throw Diagnostics::Error(std::format("External asset pack '{}' has incomplete provenance metadata.", pack.id));
		}
		if (pack.external && !pack.sourcePageUrl.starts_with("https://"))
		{
			throw Diagnostics::Error(std::format("External asset pack '{}' must use an HTTPS source page URL.", pack.id));
		}
		if (!pack.extractionPath.empty() && !Paths::IsUnderRoot(pack.rootPath, pack.extractionPath))
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' root must remain within its extraction root.", pack.id));
		}
		const std::filesystem::path archiveNamePath(pack.archiveName);
		if (pack.downloadSupported
		    && (archiveNamePath == "." || archiveNamePath == ".." || archiveNamePath.has_root_name() || archiveNamePath.has_root_directory()
		        || archiveNamePath.filename() != archiveNamePath))
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' archive name must not contain a path.", pack.id));
		}
		if (pack.downloadSupported && !archiveNames.insert(pack.archiveName).second)
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack archive name '{}' is duplicated.", pack.archiveName));
		}
		if (pack.downloadSupported)
		{
			downloadablePacks.push_back(&pack);
		}
		std::unordered_set<std::string_view> ancestors;
		const ProjectAssetPack* ancestor = &pack;
		while (!ancestor->parentPackId.empty())
		{
			if (!ancestors.insert(ancestor->id).second)
			{
				throw Diagnostics::Error(std::format("Asset pack '{}' has a cyclic parent chain.", pack.id));
			}
			ancestor = &m_catalog.assetPacks.at(ancestor->parentPackId);
			if (pack.runtimeSupported && !ancestor->runtimeSupported)
			{
				throw Diagnostics::Error(
				    std::format("Runtime-supported asset pack '{}' depends on runtime-unsupported parent '{}'.", pack.id, ancestor->id));
			}
		}
	}

	for (std::size_t leftIndex = 0; leftIndex < downloadablePacks.size(); ++leftIndex)
	{
		for (std::size_t rightIndex = leftIndex + 1; rightIndex < downloadablePacks.size(); ++rightIndex)
		{
			const ProjectAssetPack& left = *downloadablePacks[leftIndex];
			const ProjectAssetPack& right = *downloadablePacks[rightIndex];
			if (Paths::IsUnderRoot(left.extractionPath, right.extractionPath)
			    || Paths::IsUnderRoot(right.extractionPath, left.extractionPath))
			{
				throw Diagnostics::Error(
				    std::format("Downloadable asset packs '{}' and '{}' have overlapping extraction roots.", left.id, right.id));
			}
		}
	}
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

bool ProjectLevelCatalogReader::IsSafeIdentifier(std::string_view value) noexcept
{
	return !value.empty()
	    && std::all_of(
	        value.begin(),
	        value.end(),
	        [](unsigned char character)
	        {
		        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')
		            || (character >= '0' && character <= '9') || character == '-' || character == '_';
	        });
}

bool ProjectLevelCatalogReader::IsSha256(std::string_view value) noexcept
{
	return value.size() == 64
	    && std::all_of(
	        value.begin(),
	        value.end(),
	        [](unsigned char character) { return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f'); });
}
