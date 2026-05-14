#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedMaterialAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"

#include <string>
#include <vector>

struct CookedMaterialAssetBuild
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	Assets::CookedMaterialAssetHeader header;
	std::string name;
	std::vector<Assets::CookedTextureReference> textureReferences;
};

struct MaterialCookOutput final
{
	std::vector<CookedMaterialAssetBuild> assets;
	std::vector<Assets::CookedSceneMaterialAssetRef> assetReferences;
};
