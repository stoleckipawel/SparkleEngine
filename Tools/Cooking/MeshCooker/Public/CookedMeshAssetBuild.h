#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct CookedMeshAssetBuild
{
	Assets::CookedAssetId assetId = Assets::InvalidCookedAssetId;
	std::string displayName;
	std::filesystem::path sourcePath;
	Assets::CookedMeshAssetKind assetKind = Assets::CookedMeshAssetKind::Static;
	std::vector<Assets::CookedMeshVertex> vertices;
	std::vector<std::uint32_t> indices;
	std::vector<Assets::CookedMeshSkinInfluence> skinInfluences;
	std::vector<Assets::CookedMeshMorphTargetRecord> morphTargets;
	std::vector<Assets::CookedMeshMorphTargetDelta> morphTargetDeltas;

	bool HasSkinInfluences() const noexcept { return !skinInfluences.empty(); }
	bool HasMorphTargets() const noexcept { return !morphTargets.empty(); }
	bool IsSkeletal() const noexcept { return assetKind == Assets::CookedMeshAssetKind::Skeletal; }
};

struct MeshCookOutput final
{
	std::vector<CookedMeshAssetBuild> assets;
	std::vector<Assets::CookedSceneMeshAssetRef> assetReferences;
};
