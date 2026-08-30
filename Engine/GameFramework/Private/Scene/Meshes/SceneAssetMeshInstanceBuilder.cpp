#include "PCH.h"
#include "Scene/Meshes/SceneAssetMeshInstanceBuilder.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "World/Resources/MaterialResourceStore.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

static const auto g_sceneAssetMeshInstanceBuilderLogger = Logging::GetOrCreateLogger("GameFramework.SceneAssetMeshInstanceBuilder");

class SceneAssetMaterialResolution final
{
public:
	static MaterialHandle ResolveMaterial(
	    MaterialHandle payloadMaterial,
	    MaterialHandle materialBaseHandle,
	    MaterialResourceStore& materials)
	{
		if (!payloadMaterial.IsValid())
		{
			return materials.GetOrCreateDefault();
		}
		if (!materialBaseHandle.IsValid())
		{
			Diagnostics::Fatal(
			    g_sceneAssetMeshInstanceBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "A mesh instance references a material in a payload with no material block.");
		}
		return MaterialHandle(materialBaseHandle.GetIndex() + payloadMaterial.GetIndex(), materialBaseHandle.GetGeneration());
	}

	static MaterialHandle ResolveOptionalMaterial(MaterialHandle payloadMaterial, MaterialHandle materialBaseHandle)
	{
		if (!payloadMaterial.IsValid())
		{
			return MaterialHandle::Invalid();
		}
		if (!materialBaseHandle.IsValid())
		{
			Diagnostics::Fatal(
			    g_sceneAssetMeshInstanceBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "A mesh instance group references a material in a payload with no material block.");
		}
		return MaterialHandle(materialBaseHandle.GetIndex() + payloadMaterial.GetIndex(), materialBaseHandle.GetGeneration());
	}

	static Assets::CookedAssetId ResolveGroupAsset(const SceneAssetPayload& payload, const SceneAssetPayload::MeshInstanceGroup& group)
	{
		if (group.meshAssetKind == Assets::CookedMeshAssetKind::Skeletal)
		{
			if (group.meshAssetIndex >= payload.skeletalMeshAssets.size())
			{
				Diagnostics::Fatal(
				    g_sceneAssetMeshInstanceBuilderLogger,
				    __FILE__,
				    __LINE__,
				    "A skeletal mesh instance group references an absent mesh asset.");
			}
			return payload.skeletalMeshAssets[group.meshAssetIndex].assetId;
		}
		if (group.meshAssetKind != Assets::CookedMeshAssetKind::Static)
		{
			Diagnostics::Fatal(
			    g_sceneAssetMeshInstanceBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "A mesh instance group has an unsupported mesh kind.");
		}
		if (group.meshAssetIndex >= payload.staticMeshAssets.size())
		{
			Diagnostics::Fatal(
			    g_sceneAssetMeshInstanceBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "A static mesh instance group references an absent mesh asset.");
		}
		return payload.staticMeshAssets[group.meshAssetIndex].assetId;
	}
};

namespace SceneAssetMeshInstanceBuilder
{
	std::vector<ECS::SceneMeshInstanceData> BuildInstances(
	    SceneAssetPayload& payload,
	    MaterialResourceStore& materials,
	    MaterialHandle materialBaseHandle,
	    SceneMeshInstanceGroupIndex groupBaseIndex)
	{
		std::vector<ECS::SceneMeshInstanceData> instances;
		instances.reserve(payload.staticMeshInstances.size() + payload.skeletalMeshInstances.size());
		for (SceneAssetPayload::StaticMeshInstance& instance : payload.staticMeshInstances)
		{
			if (instance.meshAssetIndex >= payload.staticMeshAssets.size())
			{
				Diagnostics::Fatal(
				    g_sceneAssetMeshInstanceBuilderLogger,
				    __FILE__,
				    __LINE__,
				    "A static mesh instance references an absent mesh asset.");
			}
			const SceneAssetPayload::StaticMeshAsset& asset = payload.staticMeshAssets[instance.meshAssetIndex];
			const SceneMeshInstanceGroupIndex groupIndex = instance.groupIndex == kInvalidSceneMeshInstanceGroupIndex
			    ? kInvalidSceneMeshInstanceGroupIndex
			    : groupBaseIndex + instance.groupIndex;
			instances.push_back(
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
			if (instance.meshAssetIndex >= payload.skeletalMeshAssets.size() || instance.skeletonAssetId == Assets::InvalidCookedAssetId)
			{
				Diagnostics::Fatal(
				    g_sceneAssetMeshInstanceBuilderLogger,
				    __FILE__,
				    __LINE__,
				    "A skeletal mesh instance has an invalid mesh or skeleton asset identity.");
			}
			SceneAssetPayload::SkeletalMeshAsset& asset = payload.skeletalMeshAssets[instance.meshAssetIndex];
			instances.push_back(
			    ECS::SceneMeshInstanceData{
			        .Resource = std::make_unique<SkeletalCookedMesh>(SkeletalMeshData(std::move(asset.mesh)), asset.assetId),
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
		return instances;
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
			        .firstInstance = source.firstInstance == kInvalidSceneMeshInstanceIndex ? kInvalidSceneMeshInstanceIndex
			                                                                                : meshBaseIndex + source.firstInstance,
			        .instanceCount = source.instanceCount,
			        .groupKind = source.groupKind,
			        .flags = source.flags});
		}
		return groups;
	}
}
