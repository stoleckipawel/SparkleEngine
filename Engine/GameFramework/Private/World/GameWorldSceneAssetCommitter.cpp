#include "PCH.h"

#include "World/GameWorldSceneAssetCommitter.h"

#include "Assets/SceneAssetPayload.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Scene/Camera/SceneCameraEntry.h"
#include "Scene/Materials/SceneMaterialVariantTranslator.h"
#include "Scene/Meshes/SceneAssetMeshInstanceBuilder.h"
#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"

#include <utility>

static const auto g_sceneAssetCommitterLogger = Logging::GetOrCreateLogger("GameFramework.SceneAssetCommitter");

GameWorldSceneAssetCommitter::GameWorldSceneAssetCommitter(ECS::GameWorldState& world, GameWorldResourceStores& resources) noexcept :
    m_state(world),
    m_resources(resources)
{
}

void GameWorldSceneAssetCommitter::Commit(SceneAssetPayload&& sceneAssetPayload)
{
	if (!sceneAssetPayload.HasMeshes() && sceneAssetPayload.cameras.empty() && sceneAssetPayload.lights.empty()
	    && sceneAssetPayload.skeletons.empty() && sceneAssetPayload.animations.empty() && sceneAssetPayload.materials.empty()
	    && sceneAssetPayload.materialVariants.empty())
	{
		Diagnostics::Fatal(
		    g_sceneAssetCommitterLogger,
		    __FILE__,
		    __LINE__,
		    "A validated scene asset payload contains no resources or entities.");
	}
	const bool hasSkeletons = !sceneAssetPayload.skeletons.empty();
	const bool hasMaterials = !sceneAssetPayload.materials.empty();

	if (!sceneAssetPayload.skeletons.empty())
		m_resources.Skeletons.Append(std::move(sceneAssetPayload.skeletons));
	if (!sceneAssetPayload.animations.empty())
		m_state.AppendAnimationClips(
		    std::move(sceneAssetPayload.animations),
		    m_resources.AnimationClips,
		    sceneAssetPayload.authoredInstanceId);
	if (!sceneAssetPayload.materials.empty())
		m_resources.Textures.AppendMaterialReferences(sceneAssetPayload.materials);

	const MaterialHandle materialBaseHandle = sceneAssetPayload.materials.empty()
	    ? MaterialHandle::Invalid()
	    : m_resources.Materials.Append(std::move(sceneAssetPayload.materials));
	const auto sceneMeshBaseIndex = static_cast<SceneMeshInstanceIndex>(m_state.GetMeshCount());
	const auto sceneGroupBaseIndex = static_cast<SceneMeshInstanceGroupIndex>(m_state.GetMeshInstanceGroupCount());

	std::vector<ECS::SceneMeshInstanceData> meshInstances =
	    SceneAssetMeshInstanceBuilder::BuildInstances(sceneAssetPayload, m_resources.Materials, materialBaseHandle, sceneGroupBaseIndex);
	for (const ECS::SceneMeshInstanceData& meshInstance : meshInstances)
	{
		if (!m_resources.Materials.Contains(meshInstance.Material))
		{
			Diagnostics::Fatal(g_sceneAssetCommitterLogger, __FILE__, __LINE__, "A validated mesh instance resolved an absent material.");
		}
	}

	for (ECS::SceneMeshInstanceData& meshInstance : meshInstances)
	{
		if (!m_state.AddMesh(std::move(meshInstance)).IsValid())
		{
			Diagnostics::Fatal(g_sceneAssetCommitterLogger, __FILE__, __LINE__, "The staged world rejected a mesh instance.");
		}
	}

	m_state.AppendMeshInstanceGroups(SceneAssetMeshInstanceBuilder::BuildGroups(sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex));
	m_resources.MaterialVariants.Append(
	    SceneMaterialVariantTranslator::BuildDescriptions(sceneAssetPayload),
	    SceneMaterialVariantTranslator::BuildBindings(sceneAssetPayload, materialBaseHandle, sceneMeshBaseIndex, m_state));

	for (SceneAssetPayload::Camera& camera : sceneAssetPayload.cameras)
	{
		SceneCameraEntry sceneCamera;
		sceneCamera.name = std::move(camera.name);
		sceneCamera.desc = camera.desc;
		if (!m_state.AddCamera(std::move(sceneCamera)).IsValid())
		{
			Diagnostics::Fatal(g_sceneAssetCommitterLogger, __FILE__, __LINE__, "The staged world rejected a scene camera.");
		}
	}
	for (SceneLightDesc& light : sceneAssetPayload.lights)
	{
		if (!m_state.AddLight(std::move(light)).IsValid())
		{
			Diagnostics::Fatal(g_sceneAssetCommitterLogger, __FILE__, __LINE__, "The staged world rejected a scene light.");
		}
	}

	if (hasSkeletons)
		m_state.NotifyResourceChanged(WorldDataKind::Skeleton);
	if (hasMaterials)
	{
		m_state.NotifyResourceChanged(WorldDataKind::Material);
		m_state.NotifyResourceChanged(WorldDataKind::Texture);
	}
}
