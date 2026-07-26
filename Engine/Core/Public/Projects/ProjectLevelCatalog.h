#pragma once

#include "Core/Public/CoreAPI.h"

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
	bool defaultIncluded = false;
	bool required = false;
	bool startupDefault = false;
};

struct SPARKLE_CORE_API ProjectOptionalContentPack final
{
	std::string id;
	std::string displayName;
	std::filesystem::path rootPath;
	bool available = true;
	bool external = false;
};

struct SPARKLE_CORE_API ProjectLevelCatalog final
{
	std::vector<ProjectLevelCatalogEntry> levels;
	std::map<std::string, ProjectOptionalContentPack, std::less<>> optionalContentPacks;

	bool IsOptionalContentPackReady(
	    const std::filesystem::path& projectRoot,
	    std::string_view packId) const;
	bool IsLevelReady(
	    const std::filesystem::path& projectRoot,
	    const ProjectLevelCatalogEntry& level) const;
};

class SPARKLE_CORE_API ProjectLevelCatalogFile final
{
  public:
	static bool Load(
	    const std::filesystem::path& projectRoot,
	    ProjectLevelCatalog& outCatalog,
	    std::string& outErrorMessage);
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
