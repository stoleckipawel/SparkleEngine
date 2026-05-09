#pragma once

#include "CookedMaterialAssetBuild.h"
#include "CookedMeshAssetBuild.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"

#include <filesystem>
#include <string>
#include <vector>

struct CookedSceneBuild
{
	std::string sceneAssetId;
	std::filesystem::path sceneManifestPath;
	Assets::CookedSceneManifestHeader manifestHeader;
	std::vector<Assets::CookedSceneMeshAssetRef> meshAssetReferences;
	std::vector<Assets::CookedSceneMaterialAssetRef> materialAssetReferences;
	std::vector<Assets::CookedSceneInstanceRecord> instances;
	std::vector<CookedMeshAssetBuild> meshAssets;
	std::vector<CookedMaterialAssetBuild> materialAssets;
	std::string errorMessage;

	bool Succeeded() const noexcept { return errorMessage.empty(); }
};
