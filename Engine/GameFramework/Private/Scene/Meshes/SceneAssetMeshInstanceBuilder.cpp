#include "PCH.h"
#include "Scene/Meshes/SceneAssetMeshInstanceBuilder.h"

#include "World/Resources/MaterialResourceStore.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

class SceneAssetMaterialResolution final
{
  public:
	static MaterialHandle ResolveMaterial(
	    MaterialHandle payloadMaterial,
	    MaterialHandle materialBaseHandle,
	    MaterialResourceStore& materials) noexcept
	{
		return payloadMaterial.IsValid() && materialBaseHandle.IsValid()
		           ? MaterialHandle(materialBaseHandle.GetIndex() + payloadMaterial.GetIndex(), materialBaseHandle.GetGeneration())
		           : materials.GetOrCreateDefault();
	}

	static MaterialHandle ResolveOptionalMaterial(MaterialHandle payloadMaterial, MaterialHandle materialBaseHandle) noexcept
	{
		return payloadMaterial.IsValid() && materialBaseHandle.IsValid()
		           ? MaterialHandle(materialBaseHandle.GetIndex() + payloadMaterial.GetIndex(), materialBaseHandle.GetGeneration())
		           : MaterialHandle::Invalid();
	}

	static Assets::CookedAssetId ResolveGroupAsset(
	    const SceneAssetPayload& payload,
	    const SceneAssetPayload::MeshInstanceGroup& group) noexcept
	{
		if (group.meshAssetKind == Assets::CookedMeshAssetKind::Skeletal)
		{
			return group.meshAssetIndex < payload.skeletalMeshAssets.size()
			           ? payload.skeletalMeshAssets[group.meshAssetIndex].assetId
			           : Assets::InvalidCookedAssetId;
		}
		return group.meshAssetIndex < payload.staticMeshAssets.size()
		           ? payload.staticMeshAssets[group.meshAssetIndex].assetId
		           : Assets::InvalidCookedAssetId;
	}
};

namespace SceneAssetMeshInstanceBuilder
{
	bool BuildInstances(
	    SceneAssetPayload& payload,
	    MaterialResourceStore& materials,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceGroupIndex groupBaseIndex,
	    std::vector<ECS::SceneMeshInstanceData>& outInstances)
	{
		outInstances.reserve(payload.staticMeshInstances.size() + payload.skeletalMeshInstances.size());
		for (SceneAssetPayload::StaticMeshInstance& instance : payload.staticMeshInstances)
		{
			if (instance.meshAssetIndex >= payload.staticMeshAssets.size())
			{
				return false;
			}
			const SceneAssetPayload::StaticMeshAsset& asset = payload.staticMeshAssets[instance.meshAssetIndex];
			const SceneMeshInstanceGroupIndex groupIndex = instance.groupIndex == kInvalidSceneMeshInstanceGroupIndex
			                                                   ? kInvalidSceneMeshInstanceGroupIndex
			                                                   : groupBaseIndex + instance.groupIndex;
			outInstances.push_back(
			    ECS::SceneMeshInstanceData{
			        .Resource = std::make_unique<CookedMesh>(MeshData(asset.mesh.geometry), asset.assetId),
			        .LocalTransform = instance.transform,
			        .Material = SceneAssetMaterialResolution::ResolveMaterial(instance.material, materialBaseHandle, materials),
			        .MeshAssetId = asset.assetId,
			        .SourceInstanceId = payload.authoredInstanceId,
			        .MeshAssetIndex = instance.meshAssetIndex,
			        .InstanceGroupIndex = groupIndex,
			        .SourceNodeIndex = instance.sourceNodeIndex,
			        .Kind = SceneMeshKind::Static});
		}

		for (SceneAssetPayload::SkeletalMeshInstance& instance : payload.skeletalMeshInstances)
		{
			if (instance.meshAssetIndex >= payload.skeletalMeshAssets.size() ||
			    instance.skeletonAssetId == Assets::InvalidCookedAssetId)
			{
				return false;
			}
			SceneAssetPayload::SkeletalMeshAsset& asset = payload.skeletalMeshAssets[instance.meshAssetIndex];
			outInstances.push_back(
			    ECS::SceneMeshInstanceData{
			        .Resource = std::make_unique<SkeletalCookedMesh>(
			            SkeletalMeshData(std::move(asset.mesh)),
			            asset.assetId),
			        .LocalTransform = instance.transform,
			        .Material = SceneAssetMaterialResolution::ResolveMaterial(instance.material, materialBaseHandle, materials),
			        .MeshAssetId = asset.assetId,
			        .SkeletonAssetId = instance.skeletonAssetId,
			        .SourceInstanceId = payload.authoredInstanceId,
			        .MeshAssetIndex = instance.meshAssetIndex,
			        .SourceNodeIndex = instance.sourceNodeIndex,
			        .Kind = SceneMeshKind::Skeletal,
			        .InitialMorphWeights = instance.morphWeights});
		}
		return true;
	}

	std::vector<SceneMeshInstanceGroupData> BuildGroups(
	    const SceneAssetPayload& payload,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceIndex meshBaseIndex)
	{
		std::vector<SceneMeshInstanceGroupData> groups;
		groups.reserve(payload.meshInstanceGroups.size());
		for (const SceneAssetPayload::MeshInstanceGroup& source : payload.meshInstanceGroups)
		{
			groups.push_back(
			    SceneMeshInstanceGroupData{
			        .meshAssetId = SceneAssetMaterialResolution::ResolveGroupAsset(payload, source),
			        .meshAssetIndex = source.meshAssetIndex,
			        .materialHandle = SceneAssetMaterialResolution::ResolveOptionalMaterial(source.material, materialBaseHandle),
			        .firstInstance = source.firstInstance == kInvalidSceneMeshInstanceIndex
			                             ? kInvalidSceneMeshInstanceIndex
			                             : meshBaseIndex + source.firstInstance,
			        .instanceCount = source.instanceCount,
			        .groupKind = source.groupKind,
			        .flags = source.flags});
		}
		return groups;
	}
}
