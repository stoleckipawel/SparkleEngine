#include "PCH.h"

#include "Assets/Payload/SceneAssetPayloadMeshAppender.h"

#include "Assets/Cooked/LoadedMeshAsset.h"
#include "Assets/Loaders/MeshAssetLoader.h"
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

		struct PayloadMeshAssetBinding
		{
			Assets::CookedMeshAssetKind kind = Assets::CookedMeshAssetKind::Static;
			SceneMeshAssetIndex payloadMeshAssetIndex = kInvalidSceneMeshAssetIndex;
		};

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
	}  // namespace

	bool SceneAssetPayloadMeshAppender::AppendMeshAssets(
	    const SceneAssetId& sceneAssetId,
	    const LoadedSceneManifest& sceneManifest,
	    SceneAssetPayload& sceneAssetPayload,
	    SceneMeshAssetIndex& outMeshAssetBaseIndex,
	    std::string& errorMessage)
	{
		MeshAssetLoader meshAssetLoader;
		outMeshAssetBaseIndex = 0;
		sceneAssetPayload.staticMeshAssets.reserve(sceneAssetPayload.staticMeshAssets.size() + sceneManifest.meshAssetReferences.size());
		sceneAssetPayload.skeletalMeshAssets.reserve(sceneAssetPayload.skeletalMeshAssets.size() + sceneManifest.meshAssetReferences.size());

		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			LoadedMeshAsset loadedMesh;
			const std::filesystem::path meshAssetPath = Paths::CookedMeshAsset(meshReference.meshAssetId);
			if (!meshAssetLoader.Load(meshAssetPath, loadedMesh, errorMessage))
			{
				errorMessage = std::format(
				    "Failed to load cooked mesh asset {} from '{}' - {}",
				    Formatting::FormatHexUInt64(meshReference.meshAssetId),
				    meshAssetPath.string(),
				    errorMessage);
				return false;
			}

			if (loadedMesh.GetAssetKind() != meshReference.meshAssetKind)
			{
				errorMessage = std::format("Cooked scene mesh asset kind does not match manifest for asset {}", Formatting::FormatHexUInt64(meshReference.meshAssetId));
				return false;
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
		std::vector<PayloadMeshAssetBinding> meshAssetBindings;
		meshAssetBindings.reserve(sceneManifest.meshAssetReferences.size());
		SceneMeshAssetIndex staticMeshIndex = 0;
		SceneMeshAssetIndex skeletalMeshIndex = 0;
		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			PayloadMeshAssetBinding binding;
			binding.kind = meshReference.meshAssetKind;
			if (meshReference.meshAssetKind == CookedMeshAssetKind::Skeletal)
			{
				binding.payloadMeshAssetIndex = skeletalMeshIndex++;
			}
			else
			{
				binding.payloadMeshAssetIndex = staticMeshIndex++;
			}
			meshAssetBindings.push_back(binding);
		}
		sceneAssetPayload.staticMeshInstances.reserve(sceneAssetPayload.staticMeshInstances.size() + sceneManifest.instances.size());
		sceneAssetPayload.skeletalMeshInstances.reserve(sceneAssetPayload.skeletalMeshInstances.size() + sceneManifest.instances.size());

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

			const PayloadMeshAssetBinding& binding = meshAssetBindings[instanceRecord.meshAssetIndex];
			const Transform transform(DirectX::XMLoadFloat4x4(&instanceRecord.worldTransform));
			const MaterialHandle material = instanceRecord.materialAssetIndex == kInvalidCookedMaterialAssetIndex
			                                    ? MaterialHandle::Invalid()
			                                    : MaterialHandle(materialBaseIndex + instanceRecord.materialAssetIndex);
			if (binding.kind == CookedMeshAssetKind::Skeletal)
			{
				if (instanceRecord.skeletonRefIndex == kInvalidCookedSceneSkeletonRefIndex)
				{
					errorMessage = std::format("Scene manifest '{}' has a skeletal mesh instance without a skeleton binding", sceneAssetId.value);
					return false;
				}
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
				if (instanceRecord.skeletonRefIndex != kInvalidCookedSceneSkeletonRefIndex)
				{
					errorMessage = std::format("Scene manifest '{}' has a static mesh instance with a skeleton binding", sceneAssetId.value);
					return false;
				}
				SceneAssetPayload::StaticMeshInstance staticMeshInstance;
				staticMeshInstance.meshAssetIndex = binding.payloadMeshAssetIndex;
				staticMeshInstance.transform = transform;
				staticMeshInstance.material = material;
				staticMeshInstance.sourceNodeIndex = instanceRecord.sourceNodeIndex;
				staticMeshInstance.groupIndex = instanceRecord.groupIndex == kInvalidCookedSceneInstanceGroupIndex
				                                    ? kInvalidSceneMeshInstanceGroupIndex
				                                    : groupBaseIndex + instanceRecord.groupIndex;
				sceneAssetPayload.staticMeshInstances.push_back(std::move(staticMeshInstance));
			}
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
		std::vector<PayloadMeshAssetBinding> meshAssetBindings;
		meshAssetBindings.reserve(sceneManifest.meshAssetReferences.size());
		SceneMeshAssetIndex staticMeshIndex = 0;
		SceneMeshAssetIndex skeletalMeshIndex = 0;
		for (const CookedSceneMeshAssetRef& meshReference : sceneManifest.meshAssetReferences)
		{
			PayloadMeshAssetBinding binding;
			binding.kind = meshReference.meshAssetKind;
			binding.payloadMeshAssetIndex = meshReference.meshAssetKind == CookedMeshAssetKind::Skeletal ? skeletalMeshIndex++ : staticMeshIndex++;
			meshAssetBindings.push_back(binding);
		}
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
			const PayloadMeshAssetBinding& binding = meshAssetBindings[groupRecord.meshAssetIndex];
			meshInstanceGroup.meshAssetKind = binding.kind;
			meshInstanceGroup.meshAssetIndex = meshAssetBaseIndex + binding.payloadMeshAssetIndex;
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
