#include "Assets/Import/SceneImporter.h"
#include "Cooking/CookedSceneCooker.h"

#include <filesystem>
#include <iostream>

#include <objbase.h>

int main(int argc, char** argv)
{
	const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(coInitializeResult) && coInitializeResult != RPC_E_CHANGED_MODE)
	{
		std::cerr << "AssetConverter: failed to initialize COM for source texture loading\n";
		return 4;
	}

	if (argc != 2)
	{
		if (SUCCEEDED(coInitializeResult))
		{
			CoUninitialize();
		}

		std::cerr << "Usage: AssetConverter <source-scene-path>\n";
		return 1;
	}

	const std::filesystem::path sourceScenePath(argv[1]);
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