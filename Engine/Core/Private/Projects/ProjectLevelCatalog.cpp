#include "PCH.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include "Projects/ProjectLevelCatalogEditor.h"
#include "Projects/ProjectLevelCatalogReader.h"

#include <system_error>

bool ProjectLevelCatalog::IsOptionalContentPackAcquired(const ProjectOptionalContentPack& pack) const
{
	if (pack.rootPath.empty())
	{
		return true;
	}

	const std::filesystem::path requiredPath =
	    pack.requiredRelativePath.empty() ? pack.rootPath : pack.rootPath / pack.requiredRelativePath;
	std::error_code error;
	return std::filesystem::exists(requiredPath, error) && !error;
}

bool ProjectLevelCatalog::IsOptionalContentPackReady(const std::filesystem::path& projectRoot, std::string_view packId) const
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

	const ProjectOptionalContentPack& contentPack = pack->second;
	if (!contentPack.runtimeSupported || !IsOptionalContentPackAcquired(contentPack))
	{
		return false;
	}

	return contentPack.parentPackId.empty() || IsOptionalContentPackReady(projectRoot, contentPack.parentPackId);
}

bool ProjectLevelCatalog::IsLevelReady(const std::filesystem::path& projectRoot, const ProjectLevelCatalogEntry& level) const
{
	std::error_code error;
	return !level.id.empty() && !level.sourcePath.empty() && std::filesystem::exists(level.sourcePath, error) && !error
	    && IsOptionalContentPackReady(projectRoot, level.optionalContentPackId);
}

ProjectLevelCatalog ProjectLevelCatalogFile::Load(const std::filesystem::path& projectRoot)
{
	return ProjectLevelCatalogReader::Read(projectRoot);
}

bool ProjectLevelCatalogFile::SetLevelDefaultIncluded(
    const std::filesystem::path& projectRoot,
    std::string_view levelId,
    bool included,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogEditor::SetEntryBool(projectRoot, "[Level]", levelId, "Default", included, outErrorMessage);
}

bool ProjectLevelCatalogFile::SetOptionalContentPackAvailable(
    const std::filesystem::path& projectRoot,
    std::string_view packId,
    bool available,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogEditor::SetEntryBool(projectRoot, "[OptionalPack]", packId, "Available", available, outErrorMessage);
}
