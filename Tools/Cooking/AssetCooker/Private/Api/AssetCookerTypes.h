#pragma once

#include <cstdint>

enum class AssetCookerCategory : std::uint8_t
{
	All = 0,
	Shaders,
	Textures,
	SceneAssets,
	Meshes,
	Materials
};
