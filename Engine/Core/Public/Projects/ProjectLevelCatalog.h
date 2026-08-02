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
	std::filesystem::path sourcePath;
	std::string optionalContentPackId;
	std::string family;
	std::string variantKind;
	bool defaultIncluded = false;
	bool startupDefault = false;
};

struct SPARKLE_CORE_API ProjectOptionalContentPack final
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
	std::string version;
	std::string license;
	std::string runtimeBlocker;
	std::string downloadBlocker;
	std::uintmax_t archiveBytes = 0;
	bool available = true;
	bool external = false;
	bool downloadSupported = false;
	bool runtimeSupported = true;
};

struct SPARKLE_CORE_API ProjectLevelCatalog final
{
	std::vector<ProjectLevelCatalogEntry> levels;
	std::map<std::string, ProjectOptionalContentPack, std::less<>> optionalContentPacks;

	bool IsOptionalContentPackAcquired(const ProjectOptionalContentPack& pack) const;
	bool IsOptionalContentPackReady(const std::filesystem::path& projectRoot, std::string_view packId) const;
	bool IsLevelReady(const std::filesystem::path& projectRoot, const ProjectLevelCatalogEntry& level) const;
};

class SPARKLE_CORE_API ProjectLevelCatalogFile final
{
public:
	static ProjectLevelCatalog Load(const std::filesystem::path& projectRoot);
	static bool SetLevelDefaultIncluded(
	    const std::filesystem::path& projectRoot,
	    std::string_view levelId,
	    bool included,
	    std::string& outErrorMessage);
	static bool SetOptionalContentPackAvailable(
	    const std::filesystem::path& projectRoot,
	    std::string_view packId,
	    bool available,
	    std::string& outErrorMessage);
};
