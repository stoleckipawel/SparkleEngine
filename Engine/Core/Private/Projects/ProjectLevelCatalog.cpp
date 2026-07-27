#include "PCH.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include "Projects/ProjectLevelCatalogEditor.h"
#include "Projects/ProjectLevelCatalogReader.h"

#include <system_error>

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

	std::error_code error;
	return configuredRoot.empty() ||
	       (std::filesystem::exists(root.lexically_normal(), error) && !error);
}

bool ProjectLevelCatalog::IsLevelReady(
    const std::filesystem::path& projectRoot,
    const ProjectLevelCatalogEntry& level) const
{
	std::error_code error;
	return !level.id.empty() &&
	       !level.sourcePath.empty() &&
	       std::filesystem::exists(level.sourcePath, error) &&
	       !error &&
	       IsOptionalContentPackReady(projectRoot, level.optionalContentPackId);
}

bool ProjectLevelCatalogFile::Load(
    const std::filesystem::path& projectRoot,
    ProjectLevelCatalog& outCatalog,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogReader::Read(
	    projectRoot,
	    outCatalog,
	    outErrorMessage);
}

bool ProjectLevelCatalogFile::SetLevelDefaultIncluded(
    const std::filesystem::path& projectRoot,
    std::string_view levelId,
    bool included,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogEditor::SetEntryBool(
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
	return ProjectLevelCatalogEditor::SetEntryBool(
	    projectRoot,
	    "[OptionalPack]",
	    packId,
	    "Available",
	    available,
	    outErrorMessage);
}
