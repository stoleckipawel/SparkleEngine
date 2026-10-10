#include "PCH.h"

#include "Projects/ProjectLevelCatalogValidator.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"

#include <algorithm>
#include <format>
#include <unordered_set>
#include <vector>

static bool IsSafeCatalogIdentifier(std::string_view value) noexcept
{
	return !value.empty()
	    && std::all_of(
	        value.begin(),
	        value.end(),
	        [](unsigned char character)
	        { return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9') || character == '-' || character == '_'; });
}

static bool IsSha256(std::string_view value) noexcept
{
	return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char character) { return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f'); });
}

static void ValidateLevel(const ProjectLevelCatalogEntry& level)
{
	if (!IsSafeCatalogIdentifier(level.id))
	{
		throw Diagnostics::Error(std::format("Catalog level '{}' has an unsafe identity.", level.id));
	}
	if (level.sourcePath.empty())
	{
		throw Diagnostics::Error(std::format("Catalog level '{}' has no source path.", level.id));
	}
}

static void ValidateAssetPackMetadata(const ProjectLevelCatalog& catalog, std::string_view packId, const ProjectAssetPack& pack)
{
	if (packId.empty() || pack.id != packId)
	{
		throw Diagnostics::Error("Catalog contains an invalid asset pack identity.");
	}
	if (!IsSafeCatalogIdentifier(pack.id))
	{
		throw Diagnostics::Error(std::format("Asset pack '{}' has an unsafe identity.", pack.id));
	}
	if (pack.displayName.empty())
	{
		throw Diagnostics::Error(std::format("Asset pack '{}' has no display name.", pack.id));
	}
	if (!pack.parentPackId.empty() && !catalog.assetPacks.contains(pack.parentPackId))
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
	if (pack.requiredRelativePath.empty() || pack.requiredRelativePath == "." || pack.requiredRelativePath.has_root_name() || pack.requiredRelativePath.has_root_directory()
	    || pack.requiredRelativePath.is_absolute() || pack.requiredRelativePath.generic_string().starts_with(".."))
	{
		throw Diagnostics::Error(std::format("Asset pack '{}' has an unsafe required path.", pack.id));
	}
	if (pack.downloadSupported && !pack.external)
	{
		throw Diagnostics::Error(std::format("Downloadable asset pack '{}' must be declared external.", pack.id));
	}
	const bool filesPayload = !pack.sourceFilesManifestPath.empty();
	if (pack.downloadSupported && (pack.sourceUrl.empty() || pack.extractionPath.empty() || (!filesPayload && (pack.archiveName.empty() || pack.archiveBytes == 0 || pack.archiveSha256.empty()))))
	{
		throw Diagnostics::Error(std::format("Downloadable asset pack '{}' has incomplete acquisition metadata.", pack.id));
	}
	if (filesPayload && (!pack.downloadSupported || !pack.archiveName.empty() || pack.archiveBytes != 0 || !pack.archiveSha256.empty()))
	{
		throw Diagnostics::Error(std::format("Asset pack '{}' must choose either a file manifest or an archive payload.", pack.id));
	}
	if (filesPayload && !std::filesystem::is_regular_file(pack.sourceFilesManifestPath))
	{
		throw Diagnostics::Error(std::format("Asset pack '{}' has no readable source files manifest.", pack.id));
	}
	if (filesPayload && (!pack.sourceUrl.ends_with('/') || pack.requiredRelativePath != ".sparkle-acquisition.txt"))
	{
		throw Diagnostics::Error(std::format("Loose-file asset pack '{}' needs a source directory URL and acquisition receipt.", pack.id));
	}
	if (pack.downloadSupported && !pack.sourceUrl.starts_with("https://"))
	{
		throw Diagnostics::Error(std::format("Downloadable asset pack '{}' must use an HTTPS source URL.", pack.id));
	}
	if (pack.downloadSupported && !filesPayload && !IsSha256(pack.archiveSha256))
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
		throw Diagnostics::Error(std::format("External asset pack '{}' without acquisition support must declare its blocker.", pack.id));
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
	if (pack.downloadSupported && !filesPayload
	    && (archiveNamePath == "." || archiveNamePath == ".." || archiveNamePath.has_root_name() || archiveNamePath.has_root_directory() || archiveNamePath.filename() != archiveNamePath))
	{
		throw Diagnostics::Error(std::format("Asset pack '{}' archive name must not contain a path.", pack.id));
	}
}

static void ValidateParentChain(const ProjectLevelCatalog& catalog, const ProjectAssetPack& pack)
{
	std::unordered_set<std::string_view> ancestors;
	const ProjectAssetPack* ancestor = &pack;
	while (!ancestor->parentPackId.empty())
	{
		if (!ancestors.insert(ancestor->id).second)
		{
			throw Diagnostics::Error(std::format("Asset pack '{}' has a cyclic parent chain.", pack.id));
		}
		ancestor = &catalog.assetPacks.at(ancestor->parentPackId);
		if (pack.runtimeSupported && !ancestor->runtimeSupported)
		{
			throw Diagnostics::Error(std::format("Runtime-supported asset pack '{}' depends on runtime-unsupported parent '{}'.", pack.id, ancestor->id));
		}
	}
}

static void ValidateExtractionRoots(const std::vector<const ProjectAssetPack*>& downloadablePacks)
{
	for (std::size_t leftIndex = 0; leftIndex < downloadablePacks.size(); ++leftIndex)
	{
		for (std::size_t rightIndex = leftIndex + 1; rightIndex < downloadablePacks.size(); ++rightIndex)
		{
			const ProjectAssetPack& left = *downloadablePacks[leftIndex];
			const ProjectAssetPack& right = *downloadablePacks[rightIndex];
			if (Paths::IsUnderRoot(left.extractionPath, right.extractionPath) || Paths::IsUnderRoot(right.extractionPath, left.extractionPath))
			{
				throw Diagnostics::Error(std::format("Downloadable asset packs '{}' and '{}' have overlapping extraction roots.", left.id, right.id));
			}
		}
	}
}

void ValidateProjectLevelCatalog(const ProjectLevelCatalog& catalog)
{
	if (catalog.levels.empty())
	{
		throw Diagnostics::Error("Catalog contains no levels.");
	}

	std::unordered_set<std::string_view> levelIds;
	for (const ProjectLevelCatalogEntry& level : catalog.levels)
	{
		ValidateLevel(level);
		if (!levelIds.insert(level.id).second)
		{
			throw Diagnostics::Error(std::format("Catalog level identity '{}' is duplicated.", level.id));
		}
		if (!level.assetPackId.empty() && !catalog.assetPacks.contains(level.assetPackId))
		{
			throw Diagnostics::Error(std::format("Catalog level '{}' references unknown asset pack '{}'.", level.id, level.assetPackId));
		}
	}

	std::unordered_set<std::string_view> archiveNames;
	std::vector<const ProjectAssetPack*> downloadablePacks;
	for (const auto& [packId, pack] : catalog.assetPacks)
	{
		ValidateAssetPackMetadata(catalog, packId, pack);
		if (pack.downloadSupported && !pack.archiveName.empty() && !archiveNames.insert(pack.archiveName).second)
		{
			throw Diagnostics::Error(std::format("Downloadable asset pack archive name '{}' is duplicated.", pack.archiveName));
		}
		if (pack.downloadSupported)
		{
			downloadablePacks.push_back(&pack);
		}
		ValidateParentChain(catalog, pack);
	}

	ValidateExtractionRoots(downloadablePacks);
}
