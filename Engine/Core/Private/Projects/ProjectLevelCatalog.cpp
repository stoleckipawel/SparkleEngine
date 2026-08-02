#include "PCH.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include "Projects/ProjectLevelCatalogEditor.h"
#include "Projects/ProjectLevelCatalogReader.h"

#include <system_error>
#include <unordered_set>

bool ProjectLevelCatalog::IsAssetPackPayloadPresent(const ProjectAssetPack& pack) const
{
	if (pack.rootPath.empty())
	{
		return false;
	}

	const std::filesystem::path requiredPath =
	    pack.requiredRelativePath.empty() ? pack.rootPath : pack.rootPath / pack.requiredRelativePath;
	std::error_code error;
	return std::filesystem::exists(requiredPath, error) && !error;
}

bool ProjectLevelCatalog::IsAssetPackSourceReady(std::string_view packId) const
{
	std::unordered_set<std::string_view> visitedPackIds;
	while (!packId.empty())
	{
		if (!visitedPackIds.insert(packId).second)
		{
			return false;
		}

		const auto pack = assetPacks.find(packId);
		if (pack == assetPacks.end() || !IsAssetPackPayloadPresent(pack->second))
		{
			return false;
		}
		packId = pack->second.parentPackId;
	}

	return true;
}

bool ProjectLevelCatalog::IsAssetPackReady(std::string_view packId) const
{
	if (!IsAssetPackSourceReady(packId))
	{
		return false;
	}

	while (!packId.empty())
	{
		const auto pack = assetPacks.find(packId);
		if (pack == assetPacks.end() || !pack->second.runtimeSupported)
		{
			return false;
		}
		packId = pack->second.parentPackId;
	}

	return true;
}

bool ProjectLevelCatalog::IsLevelReady(const ProjectLevelCatalogEntry& level) const
{
	std::error_code error;
	return !level.id.empty() && !level.sourcePath.empty() && std::filesystem::exists(level.sourcePath, error) && !error
	    && IsAssetPackReady(level.assetPackId);
}

ProjectLevelCatalog ProjectLevelCatalogFile::Load(const std::filesystem::path& projectRoot)
{
	return ProjectLevelCatalogReader::Read(projectRoot);
}

bool ProjectLevelCatalogFile::SetLevelSelected(
    const std::filesystem::path& projectRoot,
    std::string_view levelId,
    bool selected,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogEditor::SetLevelSelected(projectRoot, levelId, selected, outErrorMessage);
}

bool ProjectLevelCatalogFile::SetLevelsSelected(
    const std::filesystem::path& projectRoot,
    const std::vector<std::string>& levelIds,
    bool selected,
    std::string& outErrorMessage)
{
	return ProjectLevelCatalogEditor::SetLevelsSelected(projectRoot, levelIds, selected, outErrorMessage);
}
