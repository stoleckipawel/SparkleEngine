#include "PCH.h"
#include "Scene/GameSceneAssetPayloadAppender.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Camera/SceneCameras.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Materials/SceneMaterials.h"
#include "Scene/Meshes/SceneAssetMeshComponentFactory.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Scene/Skeletons/SceneSkeletons.h"
#include "Scene/Textures/SceneTextures.h"

#include <memory>
#include <utility>

GameSceneAssetPayloadAppender::GameSceneAssetPayloadAppender(
    SceneCameras& cameras,
    SceneLighting& lighting,
    SceneMaterials& materials,
    SceneMeshes& meshes,
    SceneSkeletons& skeletons,
    SceneTextures& textures) noexcept :
    m_cameras(cameras), m_lighting(lighting), m_materials(materials), m_meshes(meshes), m_skeletons(skeletons), m_textures(textures)
{
}

bool GameSceneAssetPayloadAppender::Append(SceneAssetPayload&& sceneAssetPayload)
{
	if (!sceneAssetPayload.HasMeshes() && sceneAssetPayload.cameras.empty() && sceneAssetPayload.lights.empty() &&
	    sceneAssetPayload.skeletons.empty())
	{
		return false;
	}

	if (!sceneAssetPayload.skeletons.empty())
	{
		m_skeletons.AppendSkeletons(std::move(sceneAssetPayload.skeletons));
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
	if (!SceneAssetMeshComponentFactory::BuildMeshComponents(
	        sceneAssetPayload,
	        m_materials,
	        materialBaseHandle,
	        sceneGroupBaseIndex,
	        meshComponents))
	{
		return false;
	}

	m_meshes.AppendMeshComponents(std::move(meshComponents));
	m_meshes.AppendMeshInstanceGroups(
	    SceneAssetMeshComponentFactory::BuildMeshInstanceGroups(sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex));

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
