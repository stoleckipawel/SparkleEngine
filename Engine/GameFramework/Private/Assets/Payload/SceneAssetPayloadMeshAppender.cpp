#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMeshAppender.h"

#include "Assets/Cooked/LoadedMeshAsset.h"
#include "Assets/Loaders/MeshAssetLoader.h"
#include "Assets/Loaders/CookedAssetFileSet.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Scene/Transform.h"

#include <DirectXMath.h>

#include <filesystem>
#include <format>
#include <vector>
#include <utility>

namespace Assets
{

	SceneMeshInstanceGroupKind ToSceneMeshInstanceGroupKind(CookedSceneInstanceGroupKind groupKind) noexcept
	{
		switch (groupKind)
		{
			case CookedSceneInstanceGroupKind::SharedMeshReference:
				return SceneMeshInstanceGroupKind::SharedMeshReference;
			case CookedSceneInstanceGroupKind::AuthoredInstanceGroup:
				return SceneMeshInstanceGroupKind::AuthoredInstanceGroup;
			case CookedSceneInstanceGroupKind::None:
			default:
				return SceneMeshInstanceGroupKind::None;
		}
	}

	std::vector<float> ResolveMorphWeights(const LoadedSceneManifest& sceneManifest, const CookedSceneInstanceRecord& instanceRecord)
	{
		if (instanceRecord.firstMorphWeight == kInvalidCookedSceneMorphWeightIndex || instanceRecord.morphWeightCount == 0u)
		{
			return {};
		}

		const auto first = sceneManifest.morphWeights.begin() + instanceRecord.firstMorphWeight;
		const auto last = first + instanceRecord.morphWeightCount;
		return std::vector<float>(first, last);
	}

	void SceneAssetPayloadMeshAppender::AppendMeshAssets(
	    const LoadedSceneManifest& sceneManifest,
	    const CookedAssetFileSet& files,
	    SceneAssetPayload& sceneAssetPayload)
	{
		MeshAssetLoader meshAssetLoader;
		sceneAssetPayload.staticMeshAssets.reserve(sceneAssetPayload.staticMeshAssets.size() + sceneManifest.meshAssetReferences.size());
		sceneAssetPayload.skeletalMeshAssets.reserve(
		    sceneAssetPayload.skeletalMeshAssets.size() + sceneManifest.meshAssetReferences.size());

		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			const std::filesystem::path meshAssetPath = Paths::CookedMeshAsset(meshReference.meshAssetId);
			LoadedMeshAsset loadedMesh = meshAssetLoader.Decode(meshAssetPath, files.Get(meshAssetPath));

			if (loadedMesh.GetAssetKind() != meshReference.meshAssetKind)
			{
				throw Diagnostics::Error(std::format(
				    "Cooked scene mesh asset kind does not match manifest for asset {}",
				    Formatting::FormatHexUInt64(meshReference.meshAssetId)));
			}

			if (loadedMesh.IsSkeletal())
			{
				SceneAssetPayload::SkeletalMeshAsset skeletalMesh;
				skeletalMesh.mesh = std::move(loadedMesh.AsSkeletal());
				skeletalMesh.assetId = meshReference.meshAssetId;
				sceneAssetPayload.skeletalMeshAssets.push_back(std::move(skeletalMesh));
			}
			else
			{
				SceneAssetPayload::StaticMeshAsset staticMesh;
				staticMesh.mesh = std::move(loadedMesh.AsStatic());
				staticMesh.assetId = meshReference.meshAssetId;
				sceneAssetPayload.staticMeshAssets.push_back(std::move(staticMesh));
			}
		}

	}

	void SceneAssetPayloadMeshAppender::AppendMeshInstances(
	    const LoadedSceneManifest& sceneManifest,
	    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
	    SceneAssetPayload& sceneAssetPayload)
	{
		sceneAssetPayload.staticMeshInstances.reserve(sceneAssetPayload.staticMeshInstances.size() + sceneManifest.instances.size());
		sceneAssetPayload.skeletalMeshInstances.reserve(sceneAssetPayload.skeletalMeshInstances.size() + sceneManifest.instances.size());

		for (const CookedSceneInstanceRecord& instanceRecord : sceneManifest.instances)
		{
			const SceneAssetPayloadMeshBinding& binding = meshAssetBindings[instanceRecord.meshAssetIndex];
			const Transform transform(DirectX::XMLoadFloat4x4(&instanceRecord.worldTransform));
			const MaterialHandle material = instanceRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                                    ? MaterialHandle::Invalid()
			                                    : MaterialHandle(instanceRecord.materialAssetIndex);
			if (binding.kind == CookedMeshAssetKind::Skeletal)
			{
				SceneAssetPayload::SkeletalMeshInstance skeletalMeshInstance;
				skeletalMeshInstance.meshAssetIndex = binding.payloadMeshAssetIndex;
				skeletalMeshInstance.transform = transform;
				skeletalMeshInstance.material = material;
				skeletalMeshInstance.skeletonAssetId = sceneManifest.skeletonRefs[instanceRecord.skeletonRefIndex].skeletonAssetId;
				skeletalMeshInstance.sourceNodeIndex = instanceRecord.sourceNodeIndex;
				skeletalMeshInstance.morphWeights = ResolveMorphWeights(sceneManifest, instanceRecord);
				sceneAssetPayload.skeletalMeshInstances.push_back(std::move(skeletalMeshInstance));
			}
			else
			{
				SceneAssetPayload::StaticMeshInstance staticMeshInstance;
				staticMeshInstance.meshAssetIndex = binding.payloadMeshAssetIndex;
				staticMeshInstance.transform = transform;
				staticMeshInstance.material = material;
				staticMeshInstance.sourceNodeIndex = instanceRecord.sourceNodeIndex;
				staticMeshInstance.groupIndex = instanceRecord.groupIndex == kInvalidCookedSceneInstanceGroupIndex
				                                    ? kInvalidSceneMeshInstanceGroupIndex
				                                    : instanceRecord.groupIndex;
				sceneAssetPayload.staticMeshInstances.push_back(std::move(staticMeshInstance));
			}
		}

	}

	void SceneAssetPayloadMeshAppender::AppendMeshInstanceGroups(
	    const LoadedSceneManifest& sceneManifest,
	    std::span<const SceneAssetPayloadMeshBinding> meshAssetBindings,
	    SceneAssetPayload& sceneAssetPayload)
	{
		sceneAssetPayload.meshInstanceGroups.reserve(sceneAssetPayload.meshInstanceGroups.size() + sceneManifest.instanceGroups.size());

		for (const CookedSceneInstanceGroupRecord& groupRecord : sceneManifest.instanceGroups)
		{
			SceneAssetPayload::MeshInstanceGroup meshInstanceGroup;
			const SceneAssetPayloadMeshBinding& binding = meshAssetBindings[groupRecord.meshAssetIndex];
			meshInstanceGroup.meshAssetKind = binding.kind;
			meshInstanceGroup.meshAssetIndex = binding.payloadMeshAssetIndex;
			meshInstanceGroup.material = groupRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                                 ? MaterialHandle::Invalid()
			                                 : MaterialHandle(groupRecord.materialAssetIndex);
			meshInstanceGroup.firstInstance = groupRecord.firstInstance;
			meshInstanceGroup.instanceCount = groupRecord.instanceCount;
			meshInstanceGroup.groupKind = ToSceneMeshInstanceGroupKind(groupRecord.groupKind);
			meshInstanceGroup.flags = groupRecord.flags;
			sceneAssetPayload.meshInstanceGroups.push_back(meshInstanceGroup);
		}

	}
}
