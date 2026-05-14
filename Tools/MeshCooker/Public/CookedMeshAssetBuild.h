#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct CookedMeshAssetBuild
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	std::string displayName;
	std::filesystem::path sourcePath;
	std::vector<Assets::CookedMeshVertex> vertices;
	std::vector<std::uint32_t> indices;
};

struct MeshCookOutput final
{
	std::vector<CookedMeshAssetBuild> assets;
	std::vector<Assets::CookedSceneMeshAssetRef> assetReferences;
};
