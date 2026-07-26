#pragma once

#include "CookedSceneBuild.h"
#include "Core/Public/Files/FileUtils.h"
#include "SourceImportResult.h"

#include <filesystem>
#include <span>
#include <string>
#include <vector>

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

private:
	static bool ResolveSourceScenePath(
	    const std::filesystem::path& sourceScenePath,
	    std::filesystem::path& outResolvedPath,
	    std::string& outErrorMessage);
	static bool BuildSceneAssetId(
	    const std::filesystem::path& resolvedSourceScenePath,
	    std::string& outSceneAssetId,
	    std::string& outErrorMessage);
	static bool StageManifest(
	    const CookedSceneBuild& build,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static bool StageRegistry(
	    std::span<const CookedSceneBuild* const> builds,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static bool ResolveManifestRelativePath(
	    const CookedSceneBuild& build,
	    std::filesystem::path& outRelativePath,
	    std::string& outErrorMessage);
};
