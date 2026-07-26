#pragma once

#include <cstdint>

enum AssetCookerCategory : std::uint32_t
{
	AssetCookerCategory_All = 0,
	AssetCookerCategory_Shaders = 1,
	AssetCookerCategory_Textures = 2,
	AssetCookerCategory_SceneAssets = 3,
	AssetCookerCategory_Meshes = 4,
	AssetCookerCategory_Materials = 5
};
