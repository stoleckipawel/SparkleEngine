#pragma once

#include "Api/AssetCookerTypes.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

enum class AssetCookerPlanStep : std::uint8_t
{
	Shaders,
	Textures,
	SceneAssets
};

struct AssetCookerSceneEntry final
{
	std::string relativePath;
	std::filesystem::path sourcePath;
};

struct AssetCookerProjectCookPlan final
{
	std::string projectName;
	std::filesystem::path projectRoot;
	std::filesystem::path cookedRoot;
	std::filesystem::path shaderCompilerPath;
	std::filesystem::path textureCookerPath;
	std::filesystem::path temporaryRoot;
	std::vector<AssetCookerSceneEntry> sceneEntries;
	std::vector<AssetCookerPlanStep> steps;
};
