#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshKind.h"
#include "GameFramework/Public/Scene/Transform.h"
#include "GameFramework/Public/Scene/Meshes/Mesh.h"

#include <memory>
#include <vector>

namespace ECS
{
	struct SceneMeshInstanceData final
	{
		std::unique_ptr<Mesh> Resource;
		Transform LocalTransform;
		MaterialHandle Material = MaterialHandle::Invalid();
		Assets::CookedAssetId MeshAssetId = Assets::InvalidCookedAssetId;
		Assets::CookedAssetId SkeletonAssetId = Assets::InvalidCookedAssetId;
		SceneMeshAssetIndex MeshAssetIndex = kInvalidSceneMeshAssetIndex;
		SceneMeshInstanceGroupIndex InstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
		std::uint32_t SourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex;
		SceneMeshKind Kind = SceneMeshKind::Static;
		std::vector<float> InitialMorphWeights;
	};
}
