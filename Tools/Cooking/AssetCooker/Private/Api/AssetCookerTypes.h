#pragma once

#include <cstdint>

enum AssetCookerCategory : std::uint32_t
{
	AssetCookerCategory_All = 0,
	AssetCookerCategory_Shaders = 1,
	AssetCookerCategory_Textures = 2,
	AssetCookerCategory_SceneAssets = 3,
	AssetCookerCategory_Texture = 4,
	AssetCookerCategory_Shader = 5,
	AssetCookerCategory_Mesh = 6,
	AssetCookerCategory_Material = 7,
	AssetCookerCategory_Scene = 8
};

enum AssetCookerDiagnosticSeverity : std::uint32_t
{
	AssetCookerDiagnosticSeverity_Info = 0,
	AssetCookerDiagnosticSeverity_Warning = 1,
	AssetCookerDiagnosticSeverity_Error = 2
};

struct AssetCookerCapabilities
{
	std::uint32_t supportsProjectCook;
	std::uint32_t supportsSelectedRecook;
	std::uint32_t supportsShaderCook;
	std::uint32_t supportsTextureCook;
	std::uint32_t supportsSceneAssetCook;
	std::uint32_t supportsHotReloadOutputs;
};
