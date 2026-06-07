#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterialVariants.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"

#include <cstdint>
#include <string>

struct SPARKLE_ENGINE_API SceneAssetMaterialVariant
{
	std::string name;
	std::uint32_t sourceVariantIndex = 0;
};

struct SPARKLE_ENGINE_API SceneAssetMaterialVariantMapping
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Assets::CookedMeshAssetKind meshAssetKind = Assets::CookedMeshAssetKind::Static;
	SceneMaterialVariantIndex variantIndex = kInvalidSceneMaterialVariantIndex;
	MaterialHandle material;
};
