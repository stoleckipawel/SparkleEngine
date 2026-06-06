#pragma once

#include "CookedMaterialAssetBuild.h"
#include "CookedMeshAssetBuild.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

struct CookedSceneIdentity final
{
	std::string assetId;
	std::filesystem::path manifestPath;
};

struct CookedSceneManifestBuildData final
{
	Assets::CookedSceneManifestHeader header;
	std::vector<Assets::CookedSceneMeshAssetRef> meshAssetReferences;
	std::vector<Assets::CookedSceneMaterialAssetRef> materialAssetReferences;
	std::vector<Assets::CookedSceneInstanceRecord> instances;
	std::vector<Assets::CookedSceneInstanceGroupRecord> instanceGroups;
	std::vector<Assets::CookedSceneCameraRecord> cameras;
	std::vector<Assets::CookedSceneLightRecord> lights;
};

struct CookedSceneAssetOutputs final
{
	std::vector<CookedMeshAssetBuild> meshAssets;
	std::vector<CookedMaterialAssetBuild> materialAssets;
};

struct CookedSceneBuildStatus final
{
	std::string errorMessage;

	bool Succeeded() const noexcept { return errorMessage.empty(); }
};

struct CookedSceneBuild final
{
	CookedSceneIdentity identity;
	CookedSceneManifestBuildData manifest;
	CookedSceneAssetOutputs outputs;
	CookedSceneBuildStatus status;

	bool Succeeded() const noexcept { return status.Succeeded(); }

	void ApplyMeshOutput(MeshCookOutput&& meshOutput)
	{
		manifest.meshAssetReferences = std::move(meshOutput.assetReferences);
		outputs.meshAssets = std::move(meshOutput.assets);
	}

	void ApplyMaterialOutput(MaterialCookOutput&& materialOutput)
	{
		manifest.materialAssetReferences = std::move(materialOutput.assetReferences);
		outputs.materialAssets = std::move(materialOutput.assets);
	}
};
