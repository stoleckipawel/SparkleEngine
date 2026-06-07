#include "PCH.h"
#include "Scene/GameSceneAssetPayloadAppender.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Camera/SceneCameras.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Materials/SceneMaterials.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Scene/Textures/SceneTextures.h"

#include <memory>
#include <utility>

GameSceneAssetPayloadAppender::GameSceneAssetPayloadAppender(
    SceneCameras& cameras,
    SceneLighting& lighting,
    SceneMaterials& materials,
    SceneMeshes& meshes,
    SceneTextures& textures) noexcept :
    m_cameras(cameras), m_lighting(lighting), m_materials(materials), m_meshes(meshes), m_textures(textures)
{
}

bool GameSceneAssetPayloadAppender::Append(SceneAssetPayload&& sceneAssetPayload)
{
	if (!sceneAssetPayload.HasMeshes() && sceneAssetPayload.cameras.empty() && sceneAssetPayload.lights.empty())
	{
		return false;
	}

	if (!sceneAssetPayload.materials.empty())
	{
		m_textures.AppendMaterialTextureReferences(sceneAssetPayload.materials);
	}

	const MaterialHandle materialBaseHandle = sceneAssetPayload.materials.empty()
	                                              ? MaterialHandle::Invalid()
	                                              : m_materials.AppendMaterials(std::move(sceneAssetPayload.materials));
	const auto sceneMeshBaseIndex = static_cast<SceneMeshInstanceIndex>(m_meshes.GetMeshCount());
	const auto sceneGroupBaseIndex = static_cast<SceneMeshInstanceGroupIndex>(m_meshes.GetMeshInstanceGroupCount());

	std::vector<std::unique_ptr<MeshComponent>> meshComponents;
	meshComponents.reserve(sceneAssetPayload.meshInstances.size());
	for (SceneAssetPayload::MeshInstance& meshInstance : sceneAssetPayload.meshInstances)
	{
		if (meshInstance.meshAssetIndex >= sceneAssetPayload.meshAssets.size())
		{
			return false;
		}

		const SceneAssetPayload::MeshAsset& meshAsset = sceneAssetPayload.meshAssets[meshInstance.meshAssetIndex];
		MeshData meshData = meshAsset.mesh;
		auto mesh = std::make_unique<CookedMesh>(std::move(meshData), meshAsset.assetId);
		const MaterialHandle materialHandle = meshInstance.material.IsValid() && materialBaseHandle.IsValid()
		                                          ? MaterialHandle(materialBaseHandle.GetIndex() + meshInstance.material.GetIndex())
		                                          : m_materials.GetOrCreateDefaultMaterialHandle();
		const SceneMeshInstanceGroupIndex sceneGroupIndex = meshInstance.groupIndex == kInvalidSceneMeshInstanceGroupIndex
		                                                    ? kInvalidSceneMeshInstanceGroupIndex
		                                                    : sceneGroupBaseIndex + meshInstance.groupIndex;
		meshComponents.push_back(std::make_unique<MeshComponent>(
		    std::move(mesh),
		    meshInstance.transform,
		    materialHandle,
		    meshAsset.assetId,
		    meshInstance.meshAssetIndex,
		    sceneGroupIndex));
	}

	m_meshes.AppendMeshComponents(std::move(meshComponents));

	std::vector<MeshInstanceGroupSnapshot> meshInstanceGroups;
	meshInstanceGroups.reserve(sceneAssetPayload.meshInstanceGroups.size());
	for (const SceneAssetPayload::MeshInstanceGroup& payloadGroup : sceneAssetPayload.meshInstanceGroups)
	{
		MeshInstanceGroupSnapshot meshInstanceGroup;
		meshInstanceGroup.meshAssetIndex = payloadGroup.meshAssetIndex;
		meshInstanceGroup.meshAssetId = payloadGroup.meshAssetIndex < sceneAssetPayload.meshAssets.size()
		                                ? sceneAssetPayload.meshAssets[payloadGroup.meshAssetIndex].assetId
		                                : Assets::InvalidCookedAssetId;
		meshInstanceGroup.materialHandle = payloadGroup.material.IsValid() && materialBaseHandle.IsValid()
		                                       ? MaterialHandle(materialBaseHandle.GetIndex() + payloadGroup.material.GetIndex())
		                                       : MaterialHandle::Invalid();
		meshInstanceGroup.firstInstance = payloadGroup.firstInstance == kInvalidSceneMeshInstanceIndex
		                                  ? kInvalidSceneMeshInstanceIndex
		                                  : sceneMeshBaseIndex + payloadGroup.firstInstance;
		meshInstanceGroup.instanceCount = payloadGroup.instanceCount;
		meshInstanceGroup.groupKind = payloadGroup.groupKind;
		meshInstanceGroup.flags = payloadGroup.flags;
		meshInstanceGroups.push_back(meshInstanceGroup);
	}
	m_meshes.AppendMeshInstanceGroups(std::move(meshInstanceGroups));

	for (SceneAssetPayload::Camera& camera : sceneAssetPayload.cameras)
	{
		SceneCameraEntry sceneCamera;
		sceneCamera.name = std::move(camera.name);
		sceneCamera.desc = camera.desc;
		m_cameras.AppendCamera(std::move(sceneCamera));
	}

	for (SceneLightDesc& light : sceneAssetPayload.lights)
	{
		m_lighting.AppendLight(std::move(light));
	}

	return true;
}
