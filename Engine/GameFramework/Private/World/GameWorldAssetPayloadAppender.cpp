#include "PCH.h"
#include "World/GameWorldAssetPayloadAppender.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Camera/SceneCameras.h"
#include "Scene/Lighting/SceneLighting.h"
#include "Scene/Materials/SceneMaterials.h"
#include "Scene/Materials/SceneMaterialVariantPayloadApplier.h"
#include "Scene/Materials/SceneMaterialVariants.h"
#include "Scene/Meshes/SceneAssetMeshInstanceBuilder.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Scene/Skeletons/SceneSkeletons.h"
#include "Scene/Textures/SceneTextures.h"
#include "World/GameWorldState.h"

#include <memory>
#include <utility>

GameWorldAssetPayloadAppender::GameWorldAssetPayloadAppender(
    SceneCameras& cameras,
    SceneLighting& lighting,
    SceneMaterials& materials,
    SceneMaterialVariants& materialVariants,
    SceneMeshes& meshes,
    SceneSkeletons& skeletons,
    ECS::GameWorldState& world,
    SceneTextures& textures) noexcept :
    m_cameras(cameras),
    m_lighting(lighting),
    m_materials(materials),
    m_materialVariants(materialVariants),
    m_meshes(meshes),
    m_skeletons(skeletons),
	m_state(world),
    m_textures(textures)
{
}

bool GameWorldAssetPayloadAppender::Append(SceneAssetPayload&& sceneAssetPayload)
{
	if (!sceneAssetPayload.HasMeshes() && sceneAssetPayload.cameras.empty() && sceneAssetPayload.lights.empty() &&
	    sceneAssetPayload.skeletons.empty() && sceneAssetPayload.animations.empty())
	{
		return false;
	}
	const bool hasSkeletons = !sceneAssetPayload.skeletons.empty();
	const bool hasMaterials = !sceneAssetPayload.materials.empty();

	if (!sceneAssetPayload.skeletons.empty())
	{
		m_skeletons.AppendSkeletons(std::move(sceneAssetPayload.skeletons));
	}

	if (!sceneAssetPayload.animations.empty())
	{
		m_state.AppendAnimationClips(std::move(sceneAssetPayload.animations));
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

	std::vector<ECS::SceneMeshInstanceData> meshInstances;
	if (!SceneAssetMeshInstanceBuilder::BuildInstances(
	        sceneAssetPayload,
	        m_materials,
	        materialBaseHandle,
	        sceneGroupBaseIndex,
	        meshInstances))
	{
		return false;
	}

	for (ECS::SceneMeshInstanceData& meshInstance : meshInstances)
	{
		if (!m_meshes.AppendMesh(std::move(meshInstance)))
		{
			return false;
		}
	}
	m_meshes.AppendMeshInstanceGroups(
	    SceneAssetMeshInstanceBuilder::BuildGroups(sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex));
	m_materialVariants.AppendVariants(
	    SceneMaterialVariantPayloadApplier::BuildVariantDescs(sceneAssetPayload),
	    SceneMaterialVariantPayloadApplier::BuildVariantBindings(sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex));

	for (SceneAssetPayload::Camera& camera : sceneAssetPayload.cameras)
	{
		SceneCameraEntry sceneCamera;
		sceneCamera.name = std::move(camera.name);
		sceneCamera.desc = camera.desc;
		m_cameras.AddCamera(std::move(sceneCamera));
	}

	for (SceneLightDesc& light : sceneAssetPayload.lights)
	{
		m_lighting.AppendLight(std::move(light));
	}
	if (hasSkeletons)
	{
		m_state.NotifyResourceChanged(WorldDataKind::Skeleton);
	}
	if (hasMaterials)
	{
		m_state.NotifyResourceChanged(WorldDataKind::Material);
		m_state.NotifyResourceChanged(WorldDataKind::Texture);
	}

	return true;
}
