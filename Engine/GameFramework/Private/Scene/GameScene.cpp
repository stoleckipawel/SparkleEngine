#include "PCH.h"
#include "Scene/GameScene.h"

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

	result.status = GameSceneLoadStatus::Succeeded;

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Level '{}' loaded", desc.name);
	return result;
}

bool GameScene::AppendRuntimeScenePayload(RuntimeScenePayload&& runtimeScenePayload)
{
	if (!runtimeScenePayload.HasMeshes())
	{
		return false;
	}

	if (!runtimeScenePayload.materials.empty())
	{
		m_textures.AppendMaterialTextureReferences(runtimeScenePayload.materials);
	}

	const MaterialHandle materialBaseHandle = runtimeScenePayload.materials.empty()
	                                              ? MaterialHandle::Invalid()
	                                              : m_materials.AppendMaterials(std::move(runtimeScenePayload.materials));

	std::vector<std::unique_ptr<MeshComponent>> importedMeshes;
	importedMeshes.reserve(runtimeScenePayload.meshInstances.size());
	for (RuntimeScenePayload::MeshInstance& meshInstance : runtimeScenePayload.meshInstances)
	{
		auto mesh = std::make_unique<CookedMesh>(std::move(meshInstance.mesh), meshInstance.assetId);
		const MaterialHandle materialHandle = meshInstance.material.IsValid() && materialBaseHandle.IsValid()
		                                          ? MaterialHandle(materialBaseHandle.GetIndex() + meshInstance.material.GetIndex())
		                                          : m_materials.GetOrCreateDefaultMaterialHandle();
		importedMeshes.push_back(std::make_unique<MeshComponent>(std::move(mesh), meshInstance.transform, materialHandle));
	}

	m_meshes.AppendMeshComponents(std::move(importedMeshes));

	SPDLOG_LOGGER_INFO(g_gameSceneLogger, "Scene: Loaded {} meshes, {} materials", m_meshes.GetMeshCount(), m_materials.GetMaterialCount());

	return true;
}

void GameScene::Clear()
{
	m_lighting.Reset();
	m_materials.Reset();
	m_meshes.Reset();
	m_textures.Reset();
}
