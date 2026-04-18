#include "Assets/Import/SceneImporter.h"
#include "Cooking/CookedSceneCooker.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <functional>
#include <iostream>
#include <string_view>

#include <objbase.h>

namespace
{
	int RunWithImportedScene(
	    const std::filesystem::path& sourceScenePath,
	    const std::function<int(const SceneImportResult&)>& onImportedScene)
	{
		const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
		{
			std::cerr << "AssetConverter: failed to initialize COM for source texture loading\n";
			return 4;
		}

		SceneImportResult importResult = SceneImporter::Import(sourceScenePath);
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

	int RunCookScene(const std::filesystem::path& sourceScenePath)
	{
		return RunWithImportedScene(
		    sourceScenePath,
		    [&](const SceneImportResult& importResult) -> int
		    {
				Engine::AssetAuthoring::CookedSceneCooker cookedSceneCooker;
				const Engine::AssetAuthoring::CookedSceneBuild cookedSceneBuild = cookedSceneCooker.Cook(sourceScenePath, importResult);
				if (!cookedSceneBuild.Succeeded())
				{
					std::cerr << "AssetConverter: failed to cook '" << sourceScenePath.string() << "' - "
					          << cookedSceneBuild.errorMessage << "\n";
					return 3;
				}

				std::cout << "AssetConverter: imported '" << sourceScenePath.string() << "' via "
				          << GetSceneImporterTypeName(importResult.importerType) << " with " << importResult.GetMeshCount()
				          << " meshes and " << importResult.GetMaterialCount() << " materials; emitted scene asset '"
				          << cookedSceneBuild.sceneAssetId << "' to '" << cookedSceneBuild.sceneManifestPath.string() << "'\n";

				return 0;
		    });
	}

	int RunCollectTextureRequests(
	    const std::filesystem::path& sourceScenePath,
	    const std::filesystem::path& outputRequestPath)
	{
		return RunWithImportedScene(
		    sourceScenePath,
		    [&](const SceneImportResult& importResult) -> int
		    {
				Engine::AssetAuthoring::CookedSceneCooker cookedSceneCooker;
				std::vector<Engine::AssetAuthoring::TextureCookRequest> requests;
				std::string errorMessage;
				if (!cookedSceneCooker.CollectTextureCookRequests(importResult, requests, errorMessage))
				{
					std::cerr << "AssetConverter: failed to collect texture requests for '" << sourceScenePath.string() << "' - "
					          << errorMessage << "\n";
					return 5;
				}

				if (!Engine::AssetAuthoring::WriteTextureCookRequestList(outputRequestPath, requests, errorMessage))
				{
					std::cerr << "AssetConverter: failed to write texture request file '" << outputRequestPath.string() << "' - "
					          << errorMessage << "\n";
					return 6;
				}

				std::cout << "AssetConverter: collected " << requests.size() << " unique texture request(s) from '"
				          << sourceScenePath.string() << "' into '" << outputRequestPath.string() << "'\n";
				return 0;
		    });
	}

}

int main(int argc, char** argv)
{
	if (argc == 2)
	{
		const std::string_view argument(argv[1]);
		if (argument != "cook-scene")
		{
			return RunCookScene(std::filesystem::path(argv[1]));
		}
	}

	if (argc == 3)
	{
		const std::string_view command(argv[1]);
		if (command == "cook-scene")
		{
			return RunCookScene(std::filesystem::path(argv[2]));
		}
	}

	if (argc == 4)
	{
		const std::string_view command(argv[1]);
		if (command == "collect-texture-requests")
		{
			return RunCollectTextureRequests(std::filesystem::path(argv[2]), std::filesystem::path(argv[3]));
		}
	}

	std::cerr << "Usage:\n"
	          << "  AssetConverter cook-scene <source-scene-path>\n"
	          << "  AssetConverter collect-texture-requests <source-scene-path> <request-file-path>\n"
	          << "\n"
	          << "Compatibility:\n"
	          << "  AssetConverter <source-scene-path>\n";
	return 1;
}