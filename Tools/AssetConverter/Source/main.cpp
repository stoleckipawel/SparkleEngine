#include "Assets/Import/SceneImporter.h"
#include "Cooking/CookedSceneCooker.h"

#include <filesystem>
#include <iostream>
#include <string_view>

#include <objbase.h>

namespace
{
	int RunCookScene(const std::filesystem::path& sourceScenePath)
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

		Engine::AssetAuthoring::CookedSceneCooker cookedSceneCooker;
		const Engine::AssetAuthoring::CookedSceneBuild cookedSceneBuild = cookedSceneCooker.Cook(sourceScenePath, importResult);
		if (!cookedSceneBuild.Succeeded())
		{
			if (SUCCEEDED(coInitializeResult))
			{
				CoUninitialize();
			}

			std::cerr << "AssetConverter: failed to cook '" << sourceScenePath.string() << "' - "
			          << cookedSceneBuild.errorMessage << "\n";
			return 3;
		}

		std::cout << "AssetConverter: imported '" << sourceScenePath.string() << "' via "
		          << GetSceneImporterTypeName(importResult.importerType) << " with " << importResult.GetMeshCount()
		          << " meshes and " << importResult.GetMaterialCount() << " materials; emitted scene asset '"
		          << cookedSceneBuild.sceneAssetId << "' to '" << cookedSceneBuild.sceneManifestPath.string() << "'\n";

		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		return 0;
	}

}  // namespace

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

	std::cerr << "Usage:\n"
	          << "  AssetConverter cook-scene <source-scene-path>\n"
	          << "\n"
	          << "Compatibility:\n"
	          << "  AssetConverter <source-scene-path>\n";
	return 1;
}