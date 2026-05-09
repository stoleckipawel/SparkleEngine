#pragma once

#include "SourceImportResult.h"
#include "Cooking/CookedSceneCooker.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

class AssetConverterCommands final
{
public:
	static int RunCookScene(const std::filesystem::path& sourceScenePath);
	static int RunCollectTextureRequests(
	    const std::filesystem::path& sourceScenePath,
	    const std::filesystem::path& outputRequestPath);
	static int RunCookSceneList(const std::filesystem::path& sceneListPath, int totalSceneCount);
	static int RunCollectTextureRequestList(
	    const std::filesystem::path& sceneListPath,
	    const std::filesystem::path& outputRequestPath,
	    int totalSceneCount);

private:
	struct SceneListEntry final
	{
		std::string origin;
		std::string relativePath;
		std::filesystem::path scenePath;
	};

	static void PrintCookSceneSummary(
	    const std::filesystem::path& sourceScenePath,
	    const SourceImportResult& importResult,
	    const CookedSceneBuild& cookedSceneBuild);
	static void PrintCollectTextureSummary(
	    const std::filesystem::path& sourceScenePath,
	    std::size_t requestCount,
	    const std::filesystem::path& outputRequestPath);
	static bool TryParseSceneListEntry(std::string_view line, SceneListEntry& outEntry, std::string& outErrorMessage);
	static bool LoadSceneList(
	    const std::filesystem::path& sceneListPath,
	    std::vector<SceneListEntry>& outEntries,
	    std::string& outErrorMessage);
	static std::string GetSceneListEntryName(const SceneListEntry& entry);
	static bool TextureCookRequestsMatch(const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept;
	static bool AddUniqueTextureCookRequest(
	    const TextureCookRequest& request,
	    std::map<TextureAssetId, TextureCookRequest>& requestsById,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
	static bool AppendDefaultSkyTextureRequest(
	    std::map<TextureAssetId, TextureCookRequest>& requestsById,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
	static int RunWithImportedScene(
	    const std::filesystem::path& sourceScenePath,
	    const std::function<int(const SourceImportResult&)>& onImportedScene);
};



