#pragma once

#include "Assets/Cooked/CookedMeshAsset.h"
#include "Scene/Materials/MaterialHandle.h"
#include "Scene/Materials/MaterialVariant.h"
#include "Scene/Meshes/MeshInstanceGroup.h"

#include <cstdint>
#include <string>

struct SceneAssetMaterialVariant final
{
	std::string name;
	std::uint32_t sourceVariantIndex = 0;
};

struct SceneAssetMaterialVariantMapping final
{
	SceneMeshAssetIndex meshAssetIndex = kInvalidSceneMeshAssetIndex;
	Assets::CookedMeshAssetKind meshAssetKind = Assets::CookedMeshAssetKind::Static;
	MaterialVariantIndex variantIndex = kInvalidMaterialVariantIndex;
	MaterialHandle material;
};
