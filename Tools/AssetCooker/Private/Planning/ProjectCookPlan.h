#pragma once

#include "../../Public/AssetCookerTypes.h"

#include <filesystem>
#include <string>
#include <vector>

enum class AssetCookerPlanStep
{
	Shaders,
	Textures,
	SceneAssets
};

struct AssetCookerSceneEntry final
{
	std::string origin;
	std::string relativePath;
	std::filesystem::path sourcePath;
};

struct AssetCookerProjectCookPlan final
{
	std::string projectName;
	std::string configuration;
	std::string toolConfiguration;
	std::filesystem::path repositoryRoot;
	std::filesystem::path projectRoot;
	std::filesystem::path cookedRoot;
	std::filesystem::path planPath;
	std::filesystem::path summaryPath;
	std::filesystem::path textureSummaryPath;
	std::vector<AssetCookerSceneEntry> sceneEntries;
	std::vector<AssetCookerPlanStep> steps;
	int engineSceneCount = 0;
	int projectSceneCount = 0;
	int overriddenEngineSceneCount = 0;
};
