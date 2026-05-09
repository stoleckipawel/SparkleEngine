#include "PCH.h"

#include "Cli/AssetConverterCommands.h"

#include "MaterialCooker.h"
#include "MeshCooker.h"
#include "SceneCooker.h"
#include "SourceSceneImporter.h"

#include <iostream>
#include <vector>

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

CookedSceneBuild AssetConverterCommands::CookImportedScene(
    const std::filesystem::path& sourceScenePath,
    const SourceImportResult& importResult)
{
	CookedSceneBuild build;
	if (!importResult.IsValid())
	{
		build.errorMessage = "Scene import result is not valid";
		return build;
	}

	if (!SceneCooker::ResolveSceneAsset(sourceScenePath, build.sceneAssetId, build.sceneManifestPath, build.errorMessage))
	{
		return build;
	}

	MeshCooker::BuildMeshAssets(importResult, build.sceneAssetId, build.meshAssets, build.meshAssetReferences);
	if (!MaterialCooker::BuildMaterialAssets(
	        importResult,
	        build.sceneAssetId,
	        build.materialAssets,
	        build.materialAssetReferences,
	        build.errorMessage))
	{
		return build;
	}

	if (!SceneCooker::BuildManifest(importResult, build, build.errorMessage))
	{
		return build;
	}

	if (!MeshCooker::WriteMeshAssets(build.meshAssets, build.errorMessage) ||
	    !MaterialCooker::WriteMaterialAssets(build.materialAssets, build.errorMessage) ||
	    !SceneCooker::WriteSceneManifestAndRegistry(build, build.errorMessage))
	{
		return build;
	}

	return build;
}

int AssetConverterCommands::RunCookScene(const std::filesystem::path& sourceScenePath)
{
	return RunWithImportedScene(
	    sourceScenePath,
	    [&](const SourceImportResult& importResult) -> int
	    {
		    const CookedSceneBuild cookedSceneBuild = CookImportedScene(sourceScenePath, importResult);
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
		    std::vector<TextureCookRequest> requests;
		    std::string errorMessage;
		    if (!MaterialCooker::CollectTextureCookRequests(importResult, requests, errorMessage))
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



