#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

struct SPARKLE_CORE_API ProjectLevelCatalogEntry final
{
	std::string id;
	std::string displayName;
	std::string description;
	std::filesystem::path sourcePath;
	std::filesystem::path thumbnailPath;
	std::string sourcePageUrl;
	std::string assetPackId;
	std::string family;
	std::string variantKind;
	bool selected = false;
};

struct SPARKLE_CORE_API ProjectAssetPack final
{
	std::string id;
	std::string displayName;
	std::filesystem::path rootPath;
	std::filesystem::path extractionPath;
	std::filesystem::path requiredRelativePath;
	std::string parentPackId;
	std::string contentKind = "Scene";
	std::string sourceUrl;
	std::string sourcePageUrl;
	std::string archiveName;
	std::string archiveSha256;
	std::string version;
	std::string license;
	std::string runtimeBlocker;
	std::string downloadBlocker;
	std::uintmax_t archiveBytes = 0;
	bool external = false;
	bool downloadSupported = false;
	bool runtimeSupported = false;
};

struct SPARKLE_CORE_API ProjectLevelCatalog final
{
	std::vector<ProjectLevelCatalogEntry> levels;
	std::map<std::string, ProjectAssetPack, std::less<>> assetPacks;

	bool IsAssetPackPayloadPresent(const ProjectAssetPack& pack) const;
	bool IsAssetPackSourceReady(std::string_view packId) const;
	bool IsAssetPackReady(std::string_view packId) const;
	bool IsLevelReady(const ProjectLevelCatalogEntry& level) const;
};

class SPARKLE_CORE_API ProjectLevelCatalogFile final
{
public:
	static ProjectLevelCatalog Load(const std::filesystem::path& projectRoot);
	static bool SetLevelSelected(
	    const std::filesystem::path& projectRoot,
	    std::string_view levelId,
	    bool selected,
	    std::string& outErrorMessage);
	static bool SetLevelsSelected(
	    const std::filesystem::path& projectRoot,
	    const std::vector<std::string>& levelIds,
	    bool selected,
	    std::string& outErrorMessage);
};
