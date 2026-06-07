#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMeshAppender.h"

#include "Assets/Loaders/MeshAssetLoader.h"
#include "Core/Public/Formatting/HexFormat.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Scene/Transform.h"

#include <DirectXMath.h>

#include <filesystem>
#include <format>
#include <utility>

namespace Assets
{
	namespace
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
	}  // namespace

	bool SceneAssetPayloadMeshAppender::AppendMeshAssets(
	    const SceneAssetId& sceneAssetId,
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload,
	    SceneMeshAssetIndex& outMeshAssetBaseIndex,
	    std::string& errorMessage)
	{
		MeshAssetLoader meshAssetLoader;
		outMeshAssetBaseIndex = static_cast<SceneMeshAssetIndex>(sceneAssetPayload.meshAssets.size());
		sceneAssetPayload.meshAssets.reserve(sceneAssetPayload.meshAssets.size() + sceneManifest.meshAssetReferences.size());

		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			SceneAssetPayload::MeshAsset loadedMesh;
			const std::filesystem::path meshAssetPath = Paths::CookedMeshAsset(meshReference.meshAssetId);
			if (!meshAssetLoader.Load(meshAssetPath, loadedMesh.mesh, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked mesh asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(meshReference.meshAssetId),
				    meshAssetPath.string(),
				    errorMessage);
				return false;
			}

			loadedMesh.assetId = meshReference.meshAssetId;
			sceneAssetPayload.meshAssets.push_back(std::move(loadedMesh));
		}

		return true;
	}

	bool SceneAssetPayloadMeshAppender::AppendMeshInstances(
	    const SceneAssetId& sceneAssetId,
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload,
	    SceneMeshAssetIndex meshAssetBaseIndex,
	    SceneMeshInstanceGroupIndex groupBaseIndex,
	    std::uint32_t materialBaseIndex,
	    std::string& errorMessage)
	{
		sceneAssetPayload.meshInstances.reserve(sceneAssetPayload.meshInstances.size() + sceneManifest.instances.size());

		for (const CookedSceneInstanceRecord& instanceRecord : sceneManifest.instances)
		{
			if (instanceRecord.meshAssetIndex >= sceneManifest.meshAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh index {} but only {} mesh assets were declared",
				    sceneAssetId.value,
				    instanceRecord.meshAssetIndex,
				    sceneManifest.meshAssetReferences.size());
				return false;
			}

			if (instanceRecord.materialAssetIndex != kInvalidCookedMaterialAssetIndex &&
			    instanceRecord.materialAssetIndex >= sceneManifest.materialAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references material index {} but only {} material assets were loaded",
				    sceneAssetId.value,
				    instanceRecord.materialAssetIndex,
				    sceneManifest.materialAssetReferences.size());
				return false;
			}

			SceneAssetPayload::MeshInstance meshInstance;
			meshInstance.meshAssetIndex = meshAssetBaseIndex + instanceRecord.meshAssetIndex;
			meshInstance.transform = Transform(DirectX::XMLoadFloat4x4(&instanceRecord.worldTransform));
			meshInstance.material = instanceRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                            ? MaterialHandle::Invalid()
			                            : MaterialHandle(materialBaseIndex + instanceRecord.materialAssetIndex);
			meshInstance.groupIndex = instanceRecord.groupIndex == kInvalidCookedSceneInstanceGroupIndex
			                              ? kInvalidSceneMeshInstanceGroupIndex
			                              : groupBaseIndex + instanceRecord.groupIndex;
			meshInstance.skeletonAssetId = instanceRecord.skeletonRefIndex == kInvalidCookedSceneSkeletonRefIndex
			                               ? InvalidCookedAssetId
			                               : sceneManifest.skeletonRefs[instanceRecord.skeletonRefIndex].skeletonAssetId;
			sceneAssetPayload.meshInstances.push_back(std::move(meshInstance));
		}

		return true;
	}

	bool SceneAssetPayloadMeshAppender::AppendMeshInstanceGroups(
	    const SceneAssetId& sceneAssetId,
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload,
	    SceneMeshAssetIndex meshAssetBaseIndex,
	    SceneMeshInstanceIndex instanceBaseIndex,
	    std::uint32_t materialBaseIndex,
	    std::string& errorMessage)
	{
		sceneAssetPayload.meshInstanceGroups.reserve(sceneAssetPayload.meshInstanceGroups.size() + sceneManifest.instanceGroups.size());

		for (const CookedSceneInstanceGroupRecord& groupRecord : sceneManifest.instanceGroups)
		{
			if (groupRecord.meshAssetIndex >= sceneManifest.meshAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh group mesh index {} but only {} mesh assets were declared",
				    sceneAssetId.value,
				    groupRecord.meshAssetIndex,
				    sceneManifest.meshAssetReferences.size());
				return false;
			}

			if (groupRecord.materialAssetIndex != kInvalidCookedMaterialAssetIndex &&
			    groupRecord.materialAssetIndex >= sceneManifest.materialAssetReferences.size())
			{
				errorMessage = std::format(
				    "Scene manifest '{}' references mesh group material index {} but only {} material assets were loaded",
				    sceneAssetId.value,
				    groupRecord.materialAssetIndex,
				    sceneManifest.materialAssetReferences.size());
				return false;
			}

			SceneAssetPayload::MeshInstanceGroup meshInstanceGroup;
			meshInstanceGroup.meshAssetIndex = meshAssetBaseIndex + groupRecord.meshAssetIndex;
			meshInstanceGroup.material = groupRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                                  ? MaterialHandle::Invalid()
			                                  : MaterialHandle(materialBaseIndex + groupRecord.materialAssetIndex);
			meshInstanceGroup.firstInstance = instanceBaseIndex + groupRecord.firstInstance;
			meshInstanceGroup.instanceCount = groupRecord.instanceCount;
			meshInstanceGroup.groupKind = ToSceneMeshInstanceGroupKind(groupRecord.groupKind);
			meshInstanceGroup.flags = groupRecord.flags;
			sceneAssetPayload.meshInstanceGroups.push_back(meshInstanceGroup);
		}

		return true;
	}
}
