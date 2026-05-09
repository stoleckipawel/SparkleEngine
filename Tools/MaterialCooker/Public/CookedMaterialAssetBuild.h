#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedMaterialAsset.h"

#include <string>
#include <vector>

struct CookedMaterialAssetBuild
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	Assets::CookedMaterialAssetHeader header;
	std::string name;
	std::vector<Assets::CookedTextureReference> textureReferences;
};
