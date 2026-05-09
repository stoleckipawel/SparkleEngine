#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"

#include <cstdint>
#include <vector>

struct CookedMeshAssetBuild
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	std::vector<Assets::CookedMeshVertex> vertices;
	std::vector<std::uint32_t> indices;
};
