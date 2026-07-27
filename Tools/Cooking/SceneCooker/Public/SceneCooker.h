#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedSceneBuild;
struct CookedSceneIdentity;
struct SourceImportResult;

class SceneCooker final
{
  public:
	static bool ResolveSceneIdentity(
	    const std::filesystem::path& sourceScenePath,
	    CookedSceneIdentity& outIdentity,
	    std::string& outErrorMessage);
	static bool BuildManifest(
	    const SourceImportResult& importResult,
	    CookedSceneBuild& outBuild,
	    std::string& outErrorMessage);
	static bool StageManifestsAndRegistry(
	    std::span<const CookedSceneBuild* const> builds,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
};
