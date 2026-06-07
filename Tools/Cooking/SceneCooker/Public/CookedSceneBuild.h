#pragma once

#include "CookedMaterialAssetBuild.h"
#include "CookedMeshAssetBuild.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSkeletonAsset.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

struct CookedSkeletonAssetBuild final
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	std::uint32_t sourceSkinIndex = 0;
	std::filesystem::path sourcePath;
	std::vector<Assets::CookedSkeletonJointRecord> joints;
};

struct CookedAnimationAssetBuild final
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	Assets::CookedAssetId targetSkeletonAssetId = Assets::InvalidCookedAssetId;
	std::uint32_t sourceAnimationIndex = 0;
	float durationSeconds = 0.0f;
	std::string name;
	std::filesystem::path sourcePath;
	std::vector<Assets::CookedAnimationChannelRecord> channels;
	std::vector<Assets::CookedAnimationKeyframeRecord> keyframes;
};

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
	std::vector<Assets::CookedSceneSkeletonRef> skeletonRefs;
	std::vector<Assets::CookedSceneAnimationRef> animationRefs;
	std::vector<float> morphWeights;
};

struct CookedSceneAssetOutputs final
{
	std::vector<CookedMeshAssetBuild> meshAssets;
	std::vector<CookedMaterialAssetBuild> materialAssets;
	std::vector<CookedSkeletonAssetBuild> skeletonAssets;
	std::vector<CookedAnimationAssetBuild> animationAssets;
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
