#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

#include <filesystem>
#include <string>

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
	static bool WriteSceneManifestAndRegistry(const CookedSceneBuild& build, std::string& outErrorMessage);

private:
	static bool ResolveSourceScenePath(
	    const std::filesystem::path& sourceScenePath,
	    std::filesystem::path& outResolvedPath,
	    std::string& outErrorMessage);
	static bool BuildSceneAssetId(
	    const std::filesystem::path& resolvedSourceScenePath,
	    std::string& outSceneAssetId,
	    std::string& outErrorMessage);
	static bool UpdateSceneAssetRegistry(const CookedSceneBuild& build, std::string& outErrorMessage);
};
