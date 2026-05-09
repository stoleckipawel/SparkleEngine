#include "PCH.h"

#include "Cli/AssetConverterCommands.h"

#include "SourceSceneImporter.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"

#include <fstream>
#include <iostream>
#include <system_error>

#include <objbase.h>

void AssetConverterCommands::PrintCookSceneSummary(
    const std::filesystem::path& sourceScenePath,
    const SourceImportResult& importResult,
    const CookedSceneBuild& cookedSceneBuild)
{
	std::cout << "AssetConverter Summary:\n"
	          << "  mode=cook-scene\n"
	          << "  source='" << sourceScenePath.string() << "'\n"
	          << "  importer='" << GetSourceImporterTypeName(importResult.importerType) << "'\n"
	          << "  meshes=" << importResult.GetMeshCount() << "\n"
	          << "  materials=" << importResult.GetMaterialCount() << "\n"
	          << "  sceneManifest='" << cookedSceneBuild.sceneManifestPath.string() << "'\n";
}

void AssetConverterCommands::PrintCollectTextureSummary(
    const std::filesystem::path& sourceScenePath,
    std::size_t requestCount,
    const std::filesystem::path& outputRequestPath)
{
	std::cout << "AssetConverter Summary:\n"
	          << "  mode=collect-texture-requests\n"
	          << "  source='" << sourceScenePath.string() << "'\n"
	          << "  uniqueRequests=" << requestCount << "\n"
	          << "  requestFile='" << outputRequestPath.string() << "'\n";
}

bool AssetConverterCommands::TryParseSceneListEntry(
    std::string_view line,
    SceneListEntry& outEntry,
    std::string& outErrorMessage)
{
	const std::size_t firstSeparator = line.find('|');
	if (firstSeparator == std::string_view::npos)
	{
		outErrorMessage = "Scene list entry is malformed: " + std::string(line);
		return false;
	}

	const std::size_t secondSeparator = line.find('|', firstSeparator + 1);
	if (secondSeparator == std::string_view::npos)
	{
		outErrorMessage = "Scene list entry is malformed: " + std::string(line);
		return false;
	}

	outEntry.origin = std::string(line.substr(0, firstSeparator));
	outEntry.relativePath = std::string(line.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1));
	outEntry.scenePath = std::filesystem::path(std::string(line.substr(secondSeparator + 1))).lexically_normal();
	if (outEntry.origin.empty() || outEntry.relativePath.empty() || outEntry.scenePath.empty())
	{
		outErrorMessage = "Scene list entry has an empty field: " + std::string(line);
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool AssetConverterCommands::LoadSceneList(
    const std::filesystem::path& sceneListPath,
    std::vector<SceneListEntry>& outEntries,
    std::string& outErrorMessage)
{
	std::ifstream input(sceneListPath);
	if (!input.is_open())
	{
		outErrorMessage = "Failed to open scene list file '" + sceneListPath.string() + "'.";
		return false;
	}

	outEntries.clear();
	std::size_t lineNumber = 0;
	for (std::string line; std::getline(input, line);)
	{
		++lineNumber;
		if (line.empty())
		{
			continue;
		}

		SceneListEntry entry;
		if (!TryParseSceneListEntry(line, entry, outErrorMessage))
		{
			outErrorMessage += " Line " + std::to_string(lineNumber) + ".";
			return false;
		}

		outEntries.push_back(std::move(entry));
	}

	outErrorMessage.clear();
	return true;
}

std::string AssetConverterCommands::GetSceneListEntryName(const SceneListEntry& entry)
{
	return entry.origin + ":" + entry.relativePath;
}

bool AssetConverterCommands::TextureCookRequestsMatch(const TextureCookRequest& lhs, const TextureCookRequest& rhs) noexcept
{
	return lhs.assetId == rhs.assetId && lhs.sourcePath == rhs.sourcePath && lhs.outputPath == rhs.outputPath &&
	       lhs.colorSpace == rhs.colorSpace && lhs.mipPolicy == rhs.mipPolicy && lhs.mipFilter == rhs.mipFilter &&
	       lhs.colorProcessingPolicy == rhs.colorProcessingPolicy && lhs.textureGroup == rhs.textureGroup &&
	       lhs.dimension == rhs.dimension && lhs.channelMask == rhs.channelMask;
}

bool AssetConverterCommands::AddUniqueTextureCookRequest(
    const TextureCookRequest& request,
    std::map<TextureAssetId, TextureCookRequest>& requestsById,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	const auto existingRequest = requestsById.find(request.assetId);
	if (existingRequest == requestsById.end())
	{
		requestsById.emplace(request.assetId, request);
		outRequests.push_back(request);
		outErrorMessage.clear();
		return true;
	}

	if (!TextureCookRequestsMatch(existingRequest->second, request))
	{
		outErrorMessage = "Texture request conflict for asset id " + std::to_string(request.assetId) + ".";
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool AssetConverterCommands::AppendDefaultSkyTextureRequest(
    std::map<TextureAssetId, TextureCookRequest>& requestsById,
    std::vector<TextureCookRequest>& outRequests,
    std::string& outErrorMessage)
{
	const std::filesystem::path sourcePath =
	    (Paths::EngineRoot() / "Assets" / "Textures" / "Sky" / "evening_road_01_puresky_4k.exr").lexically_normal();
	std::error_code existsError;
	if (!std::filesystem::exists(sourcePath, existsError))
	{
		outErrorMessage = "Default sky source texture was not found: " + sourcePath.string();
		return false;
	}

	TextureCookRequest request;
	request.assetId = Hash::Fnv1a64("engine:linear:Assets/Textures/Sky/evening_road_01_puresky_4k.exr");
	request.sourcePath = sourcePath;
	request.outputPath = (Paths::CookedTextureRoot() / "Defaults" / "default_cubemap.stex").lexically_normal();
	request.colorSpace = TextureColorSpace::Linear;
	request.mipPolicy = TextureMipPolicy::Generate;
	request.mipFilter = TextureMipFilter::Regular;
	request.colorProcessingPolicy = TextureColorProcessingPolicy::Linear;
	request.textureGroup = TextureGroup::Default;
	request.dimension = TextureDimension::Texture2D;
	request.channelMask = TextureChannelMask::Rgba;

	if (!AddUniqueTextureCookRequest(request, requestsById, outRequests, outErrorMessage))
	{
		return false;
	}

	std::cout << "[LOG] Added default sky texture request: source='" << request.sourcePath.string() << "' output='"
	          << request.outputPath.string() << "'\n";
	outErrorMessage.clear();
	return true;
}

int AssetConverterCommands::RunWithImportedScene(
    const std::filesystem::path& sourceScenePath,
    const std::function<int(const SourceImportResult&)>& onImportedScene)
{
	const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
	{
		std::cerr << "AssetConverter: failed to initialize COM for source texture loading\n";
		return 4;
	}

	SourceImportResult importResult = SourceSceneImporter::Import(sourceScenePath);
	if (!importResult.IsValid())
	{
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		std::cerr << "AssetConverter: failed to import '" << sourceScenePath.string() << "'\n";
		return 2;
	}

	const int exitCode = onImportedScene(importResult);

	if (SUCCEEDED(coInitializeResult))
	{
		CoUninitialize();
	}

	return exitCode;
}

int AssetConverterCommands::RunCookScene(const std::filesystem::path& sourceScenePath)
{
	return RunWithImportedScene(
	    sourceScenePath,
	    [&](const SourceImportResult& importResult) -> int
	    {
		    CookedSceneCooker cookedSceneCooker;
		    const CookedSceneBuild cookedSceneBuild = cookedSceneCooker.Cook(sourceScenePath, importResult);
		    if (!cookedSceneBuild.Succeeded())
		    {
			    std::cerr << "AssetConverter: failed to cook '" << sourceScenePath.string() << "' - "
			              << cookedSceneBuild.errorMessage << "\n";
			    return 3;
		    }

		    std::cout << "AssetConverter: imported '" << sourceScenePath.string() << "' via "
		              << GetSourceImporterTypeName(importResult.importerType) << " with " << importResult.GetMeshCount()
		              << " meshes and " << importResult.GetMaterialCount() << " materials; emitted scene asset '"
		              << cookedSceneBuild.sceneAssetId << "' to '" << cookedSceneBuild.sceneManifestPath.string() << "'\n";
		    PrintCookSceneSummary(sourceScenePath, importResult, cookedSceneBuild);

		    return 0;
	    });
}

int AssetConverterCommands::RunCollectTextureRequests(
    const std::filesystem::path& sourceScenePath,
    const std::filesystem::path& outputRequestPath)
{
	return RunWithImportedScene(
	    sourceScenePath,
	    [&](const SourceImportResult& importResult) -> int
	    {
		    CookedSceneCooker cookedSceneCooker;
		    std::vector<TextureCookRequest> requests;
		    std::string errorMessage;
		    if (!cookedSceneCooker.CollectTextureCookRequests(importResult, requests, errorMessage))
		    {
			    std::cerr << "AssetConverter: failed to collect texture requests for '" << sourceScenePath.string() << "' - "
			              << errorMessage << "\n";
			    return 5;
		    }

		    if (!WriteTextureCookRequestList(outputRequestPath, requests, errorMessage))
		    {
			    std::cerr << "AssetConverter: failed to write texture request file '" << outputRequestPath.string() << "' - "
			              << errorMessage << "\n";
			    return 6;
		    }

		    std::cout << "AssetConverter: collected " << requests.size() << " unique texture request(s) from '"
		              << sourceScenePath.string() << "' into '" << outputRequestPath.string() << "'\n";
		    PrintCollectTextureSummary(sourceScenePath, requests.size(), outputRequestPath);
		    return 0;
	    });
}

int AssetConverterCommands::RunCookSceneList(const std::filesystem::path& sceneListPath, int totalSceneCount)
{
	std::vector<SceneListEntry> sceneEntries;
	std::string errorMessage;
	if (!LoadSceneList(sceneListPath, sceneEntries, errorMessage))
	{
		std::cerr << "AssetConverter: " << errorMessage << "\n";
		return 1;
	}

	int cookedSceneCount = 0;
	std::vector<std::string> failedScenes;
	for (const SceneListEntry& sceneEntry : sceneEntries)
	{
		std::cout << "\n[LOG] Cooking [" << (cookedSceneCount + 1) << "/" << totalSceneCount << "] " << sceneEntry.origin << ": "
		          << sceneEntry.relativePath << "\n";

		const int sceneExitCode = RunCookScene(sceneEntry.scenePath);
		if (sceneExitCode != 0)
		{
			std::cerr << "[ERROR] Failed to cook '" << sceneEntry.relativePath << "'.\n";
			failedScenes.push_back(GetSceneListEntryName(sceneEntry));
			continue;
		}

		++cookedSceneCount;
	}

	if (!failedScenes.empty())
	{
		std::cerr << "\n[ERROR] CookAssets completed with " << failedScenes.size() << " failed scene(s).\n";
		std::cerr << "[ERROR] Failed scenes: ";
		for (std::size_t index = 0; index < failedScenes.size(); ++index)
		{
			if (index > 0)
			{
				std::cerr << ';';
			}
			std::cerr << failedScenes[index];
		}
		std::cerr << "\n";
		return 1;
	}

	return 0;
}

int AssetConverterCommands::RunCollectTextureRequestList(
    const std::filesystem::path& sceneListPath,
    const std::filesystem::path& outputRequestPath,
    int totalSceneCount)
{
	std::vector<SceneListEntry> sceneEntries;
	std::string errorMessage;
	if (!LoadSceneList(sceneListPath, sceneEntries, errorMessage))
	{
		std::cerr << "AssetConverter: " << errorMessage << "\n";
		return 1;
	}

	std::map<TextureAssetId, TextureCookRequest> requestsById;
	std::vector<TextureCookRequest> requests;
	std::vector<std::string> failedScenes;
	int collectedSceneCount = 0;
	for (const SceneListEntry& sceneEntry : sceneEntries)
	{
		std::cout << "\n[LOG] Collecting texture requests [" << (collectedSceneCount + 1) << "/" << totalSceneCount << "] "
		          << sceneEntry.origin << ": " << sceneEntry.relativePath << "\n";

		std::vector<TextureCookRequest> sceneRequests;
		const int collectExitCode = RunWithImportedScene(
		    sceneEntry.scenePath,
		    [&](const SourceImportResult& importResult) -> int
		    {
			    CookedSceneCooker cookedSceneCooker;
			    if (!cookedSceneCooker.CollectTextureCookRequests(importResult, sceneRequests, errorMessage))
			    {
				    std::cerr << "AssetConverter: failed to collect texture requests for '" << sceneEntry.scenePath.string()
				              << "' - " << errorMessage << "\n";
				    return 5;
			    }

			    return 0;
		    });
		if (collectExitCode != 0)
		{
			std::cerr << "[ERROR] Failed to collect texture requests for '" << sceneEntry.relativePath << "'.\n";
			failedScenes.push_back(GetSceneListEntryName(sceneEntry));
			++collectedSceneCount;
			continue;
		}

		for (const TextureCookRequest& request : sceneRequests)
		{
			if (!AddUniqueTextureCookRequest(request, requestsById, requests, errorMessage))
			{
				std::cerr << "AssetConverter: " << errorMessage << "\n";
				return 7;
			}
		}

		++collectedSceneCount;
	}

	if (!failedScenes.empty())
	{
		std::cerr << "\n[ERROR] Texture request collection failed for " << failedScenes.size() << " scene(s).\n";
		std::cerr << "[ERROR] Failed scenes: ";
		for (std::size_t index = 0; index < failedScenes.size(); ++index)
		{
			if (index > 0)
			{
				std::cerr << ';';
			}
			std::cerr << failedScenes[index];
		}
		std::cerr << "\n";
		return 1;
	}

	if (!AppendDefaultSkyTextureRequest(requestsById, requests, errorMessage))
	{
		std::cerr << "AssetConverter: " << errorMessage << "\n";
		return 8;
	}

	if (!WriteTextureCookRequestList(outputRequestPath, requests, errorMessage))
	{
		std::cerr << "AssetConverter: failed to write texture request file '" << outputRequestPath.string() << "' - "
		          << errorMessage << "\n";
		return 6;
	}

	std::cout << "\n[LOG] Collected " << requests.size() << " unique texture request(s) into " << outputRequestPath.string()
	          << "\n";
	return 0;
}



