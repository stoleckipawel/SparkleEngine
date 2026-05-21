#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <functional>
#include <string>

class AssetConverterCommands final
{
public:
	static int RunCookScene(const std::filesystem::path& sourceScenePath);
	static int RunCollectTextureRequests(
	    const std::filesystem::path& sourceScenePath,
	    const std::filesystem::path& outputRequestPath);

private:
	static void PrintCookSceneSummary(
	    const std::filesystem::path& sourceScenePath,
	    const SourceImportResult& importResult,
	    const CookedSceneBuild& cookedSceneBuild);
	static void PrintCollectTextureSummary(
	    const std::filesystem::path& sourceScenePath,
	    std::size_t requestCount,
	    const std::filesystem::path& outputRequestPath);
	static int RunWithImportedScene(
	    const std::filesystem::path& sourceScenePath,
	    const std::function<int(const SourceImportResult&)>& onImportedScene);
	static CookedSceneBuild CookImportedScene(
	    const std::filesystem::path& sourceScenePath,
	    const SourceImportResult& importResult);
};



