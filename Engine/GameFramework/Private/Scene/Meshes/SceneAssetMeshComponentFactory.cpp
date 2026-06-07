#include "PCH.h"

#include "Scene/Meshes/SceneAssetMeshComponentFactory.h"

#include "Scene/Materials/SceneMaterials.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"
#include "Scene/Meshes/SkeletalMeshComponent.h"
#include "Scene/Meshes/StaticMeshComponent.h"

#include <memory>
#include <utility>

namespace SceneAssetMeshComponentFactory
{
	namespace
	{
		MaterialHandle ResolveMaterialHandle(
		    MaterialHandle payloadMaterial,
		    MaterialHandle materialBaseHandle,
		    SceneMaterials& sceneMaterials) noexcept
		{
			return payloadMaterial.IsValid() && materialBaseHandle.IsValid()
			           ? MaterialHandle(materialBaseHandle.GetIndex() + payloadMaterial.GetIndex())
			           : sceneMaterials.GetOrCreateDefaultMaterialHandle();
		}

		MaterialHandle ResolveOptionalMaterialHandle(MaterialHandle payloadMaterial, MaterialHandle materialBaseHandle) noexcept
		{
			return payloadMaterial.IsValid() && materialBaseHandle.IsValid()
			           ? MaterialHandle(materialBaseHandle.GetIndex() + payloadMaterial.GetIndex())
			           : MaterialHandle::Invalid();
		}

		Assets::CookedAssetId ResolveGroupMeshAssetId(
		    const SceneAssetPayload& sceneAssetPayload,
		    const SceneAssetPayload::MeshInstanceGroup& payloadGroup) noexcept
		{
			if (payloadGroup.meshAssetKind == Assets::CookedMeshAssetKind::Skeletal)
			{
				return payloadGroup.meshAssetIndex < sceneAssetPayload.skeletalMeshAssets.size()
				           ? sceneAssetPayload.skeletalMeshAssets[payloadGroup.meshAssetIndex].assetId
				           : Assets::InvalidCookedAssetId;
			}

			return payloadGroup.meshAssetIndex < sceneAssetPayload.staticMeshAssets.size()
			           ? sceneAssetPayload.staticMeshAssets[payloadGroup.meshAssetIndex].assetId
			           : Assets::InvalidCookedAssetId;
		}
	}  // namespace

	bool BuildMeshComponents(
	    SceneAssetPayload& sceneAssetPayload,
	    SceneMaterials& sceneMaterials,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceGroupIndex sceneGroupBaseIndex,
	    std::vector<std::unique_ptr<MeshComponent>>& outMeshComponents)
	{
		outMeshComponents.reserve(sceneAssetPayload.staticMeshInstances.size() + sceneAssetPayload.skeletalMeshInstances.size());

		for (SceneAssetPayload::StaticMeshInstance& meshInstance : sceneAssetPayload.staticMeshInstances)
		{
			if (meshInstance.meshAssetIndex >= sceneAssetPayload.staticMeshAssets.size())
			{
				return false;
			}

			const SceneAssetPayload::StaticMeshAsset& meshAsset = sceneAssetPayload.staticMeshAssets[meshInstance.meshAssetIndex];
			MeshData meshData = meshAsset.mesh;
			auto mesh = std::make_unique<CookedMesh>(std::move(meshData), meshAsset.assetId);
			const SceneMeshInstanceGroupIndex sceneGroupIndex = meshInstance.groupIndex == kInvalidSceneMeshInstanceGroupIndex
			                                                    ? kInvalidSceneMeshInstanceGroupIndex
			                                                    : sceneGroupBaseIndex + meshInstance.groupIndex;
			outMeshComponents.push_back(std::make_unique<StaticMeshComponent>(
			    std::move(mesh),
			    meshInstance.transform,
			    ResolveMaterialHandle(meshInstance.material, materialBaseHandle, sceneMaterials),
			    meshAsset.assetId,
			    meshInstance.meshAssetIndex,
			    sceneGroupIndex));
		}

		for (SceneAssetPayload::SkeletalMeshInstance& meshInstance : sceneAssetPayload.skeletalMeshInstances)
		{
			if (meshInstance.meshAssetIndex >= sceneAssetPayload.skeletalMeshAssets.size() ||
			    meshInstance.skeletonAssetId == Assets::InvalidCookedAssetId)
			{
				return false;
			}

			const SceneAssetPayload::SkeletalMeshAsset& meshAsset = sceneAssetPayload.skeletalMeshAssets[meshInstance.meshAssetIndex];
			SkeletalMeshData meshData = std::move(meshAsset.mesh);
			auto mesh = std::make_unique<SkeletalCookedMesh>(std::move(meshData), meshAsset.assetId);
			outMeshComponents.push_back(std::make_unique<SkeletalMeshComponent>(
			    std::move(mesh),
			    meshInstance.transform,
			    ResolveMaterialHandle(meshInstance.material, materialBaseHandle, sceneMaterials),
			    meshAsset.assetId,
			    meshInstance.meshAssetIndex,
			    meshInstance.skeletonAssetId));
		}

		return true;
	}

	std::vector<MeshInstanceGroupSnapshot> BuildMeshInstanceGroups(
	    const SceneAssetPayload& sceneAssetPayload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex sceneMeshBaseIndex)
	{
		std::vector<MeshInstanceGroupSnapshot> meshInstanceGroups;
		meshInstanceGroups.reserve(sceneAssetPayload.meshInstanceGroups.size());
		for (const SceneAssetPayload::MeshInstanceGroup& payloadGroup : sceneAssetPayload.meshInstanceGroups)
		{
			MeshInstanceGroupSnapshot meshInstanceGroup;
			meshInstanceGroup.meshAssetIndex = payloadGroup.meshAssetIndex;
			meshInstanceGroup.meshAssetId = ResolveGroupMeshAssetId(sceneAssetPayload, payloadGroup);
			meshInstanceGroup.materialHandle = ResolveOptionalMaterialHandle(payloadGroup.material, materialBaseHandle);
			meshInstanceGroup.firstInstance = payloadGroup.firstInstance == kInvalidSceneMeshInstanceIndex
			                                  ? kInvalidSceneMeshInstanceIndex
			                                  : sceneMeshBaseIndex + payloadGroup.firstInstance;
			meshInstanceGroup.instanceCount = payloadGroup.instanceCount;
			meshInstanceGroup.groupKind = payloadGroup.groupKind;
			meshInstanceGroup.flags = payloadGroup.flags;
			meshInstanceGroups.push_back(meshInstanceGroup);
		}

		return meshInstanceGroups;
	}
}
