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
	importedMeshes.reserve(runtimeScenePayload.meshes.size());
	for (std::size_t meshIndex = 0; meshIndex < runtimeScenePayload.meshes.size(); ++meshIndex)
	{
		auto mesh = std::make_unique<CookedMesh>(std::move(runtimeScenePayload.meshes[meshIndex]));
		const MaterialHandle localMaterialHandle = meshIndex < runtimeScenePayload.materialHandles.size()
		                                               ? runtimeScenePayload.materialHandles[meshIndex]
		                                               : MaterialHandle::Invalid();
		const Transform importedTransform =
		    meshIndex < runtimeScenePayload.transforms.size() ? runtimeScenePayload.transforms[meshIndex] : Transform();
		const MaterialHandle materialHandle = localMaterialHandle.IsValid() && materialBaseHandle.IsValid()
		                                          ? MaterialHandle(materialBaseHandle.GetIndex() + localMaterialHandle.GetIndex())
		                                          : m_materials.GetOrCreateDefaultMaterialHandle();
		importedMeshes.push_back(std::make_unique<MeshComponent>(std::move(mesh), importedTransform, materialHandle));
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
