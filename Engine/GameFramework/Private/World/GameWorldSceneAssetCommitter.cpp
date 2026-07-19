#include "PCH.h"

#include "World/GameWorldSceneAssetCommitter.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Materials/SceneMaterialVariantTranslator.h"
#include "Scene/Meshes/SceneAssetMeshInstanceBuilder.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"

#include <utility>

GameWorldSceneAssetCommitter::GameWorldSceneAssetCommitter(
    ECS::GameWorldState& world,
    GameWorldResourceStores& resources) noexcept :
    m_state(world),
    m_resources(resources)
{
}

bool GameWorldSceneAssetCommitter::Commit(SceneAssetPayload&& sceneAssetPayload)
{
	if (!sceneAssetPayload.HasMeshes() && sceneAssetPayload.cameras.empty() && sceneAssetPayload.lights.empty() &&
	    sceneAssetPayload.skeletons.empty() && sceneAssetPayload.animations.empty() && sceneAssetPayload.materials.empty() &&
	    sceneAssetPayload.materialVariants.empty())
	{
		return false;
	}
	const bool hasSkeletons = !sceneAssetPayload.skeletons.empty();
	const bool hasMaterials = !sceneAssetPayload.materials.empty();

	if (!sceneAssetPayload.skeletons.empty())
		m_resources.Skeletons.Append(std::move(sceneAssetPayload.skeletons));
	if (!sceneAssetPayload.animations.empty())
		m_state.AppendAnimationClips(std::move(sceneAssetPayload.animations));
	if (!sceneAssetPayload.materials.empty())
		m_resources.Textures.AppendMaterialReferences(sceneAssetPayload.materials);

	const MaterialHandle materialBaseHandle = sceneAssetPayload.materials.empty()
	                                              ? MaterialHandle::Invalid()
	                                              : m_resources.Materials.Append(std::move(sceneAssetPayload.materials));
	const auto sceneMeshBaseIndex = static_cast<SceneMeshInstanceIndex>(m_state.GetMeshCount());
	const auto sceneGroupBaseIndex = static_cast<SceneMeshInstanceGroupIndex>(m_state.GetMeshInstanceGroupCount());

	std::vector<ECS::SceneMeshInstanceData> meshInstances;
	if (!SceneAssetMeshInstanceBuilder::BuildInstances(
	        sceneAssetPayload,
	        m_resources.Materials,
	        materialBaseHandle,
	        sceneGroupBaseIndex,
	        meshInstances))
	{
		return false;
	}
	for (const ECS::SceneMeshInstanceData& meshInstance : meshInstances)
		if (!m_resources.Materials.Contains(meshInstance.Material))
			return false;

	for (ECS::SceneMeshInstanceData& meshInstance : meshInstances)
		if (!m_state.AddMesh(std::move(meshInstance)).IsValid())
			return false;

	m_state.AppendMeshInstanceGroups(
	    SceneAssetMeshInstanceBuilder::BuildGroups(sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex));
	m_resources.MaterialVariants.Append(
	    SceneMaterialVariantTranslator::BuildDescriptions(sceneAssetPayload),
	    SceneMaterialVariantTranslator::BuildBindings(
	        sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex, m_state));

	for (SceneAssetPayload::Camera& camera : sceneAssetPayload.cameras)
	{
		SceneCameraEntry sceneCamera;
		sceneCamera.name = std::move(camera.name);
		sceneCamera.desc = camera.desc;
		m_state.AddCamera(std::move(sceneCamera));
	}
	for (SceneLightDesc& light : sceneAssetPayload.lights)
		m_state.AddLight(std::move(light));

	if (hasSkeletons)
		m_state.NotifyResourceChanged(WorldDataKind::Skeleton);
	if (hasMaterials)
	{
		m_state.NotifyResourceChanged(WorldDataKind::Material);
		m_state.NotifyResourceChanged(WorldDataKind::Texture);
	}
	return true;
}
