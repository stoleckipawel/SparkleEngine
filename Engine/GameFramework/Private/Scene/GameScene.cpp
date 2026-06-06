#include "PCH.h"
#include "Scene/GameScene.h"

#include "Assets/SceneAssetPayload.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"

#include <memory>

static const auto g_gameSceneLogger = Logging::GetOrCreateLogger("GameFramework.GameScene");

GameScene::GameScene() = default;

GameScene::~GameScene() noexcept = default;

GameSceneLoadResult GameScene::LoadLevel(const LevelAsset& level)
{
	return LoadLevel(level.BuildDescription());
}

GameSceneLoadResult GameScene::LoadLevel(const LevelDesc& desc)
{
	GameSceneLoadResult result;

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Loading level '{}'", desc.name);

	Clear();
	m_cameras.Reset(desc.cameraDesc);

	result.status = GameSceneLoadStatus::Succeeded;

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Level '{}' loaded", desc.name);
	return result;
}

bool GameScene::AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload)
{
	if (!sceneAssetPayload.HasMeshes() && sceneAssetPayload.cameras.empty())
	{
		return false;
	}

	const SceneAssetPayloadDiagnostics diagnostics = sceneAssetPayload.diagnostics;

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
			    std::move(mesh), meshInstance.transform, materialHandle, meshAsset.assetId, meshInstance.meshAssetIndex, sceneGroupIndex));
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

	SPDLOG_LOGGER_INFO(
	    g_gameSceneLogger,
	    "Scene: Loaded {} meshes, {} materials, payload sceneAssets={}, meshAssetRefs={}, meshInstances={}, instanceGroups={}, cameras={}",
	    m_meshes.GetMeshCount(),
	    m_materials.GetMaterialCount(),
	    diagnostics.loadedSceneAssetCount,
	    diagnostics.meshAssetReferenceCount,
	    diagnostics.meshInstanceCount,
	    diagnostics.meshInstanceGroupCount,
	    diagnostics.cameraCount);

	return true;
}

GameSceneSnapshot GameScene::CaptureSnapshot() const
{
	GameSceneSnapshot snapshot;
	snapshot.camera = m_cameras.GetActiveCamera().CaptureSnapshot();
	snapshot.lighting = m_lighting.CaptureSnapshot();
	snapshot.textures = m_textures.CaptureSnapshot();
	snapshot.materials = m_materials.CaptureSnapshot();
	snapshot.meshes = m_meshes.CaptureSnapshot();
	return snapshot;
}

void GameScene::Clear()
{
	m_lighting.Reset();
	m_materials.Reset();
	m_meshes.Reset();
	m_textures.Reset();
	m_cameras.Reset();
}
