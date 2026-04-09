#include "PCH.h"
#include "Scene/GameScene.h"

#include "Scene/Meshes/ImportedMesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Core/Public/Diagnostics/Log.h"

#include <memory>

GameScene::GameScene() = default;

GameScene::~GameScene() noexcept = default;

GameSceneLoadResult GameScene::LoadLevel(const LevelAsset& level)
{
	return LoadLevel(level.BuildDescription());
}

GameSceneLoadResult GameScene::LoadLevel(const LevelDesc& desc)
{
	GameSceneLoadResult result;

	LOG_INFO("Scene: Loading level '" + desc.name + "'");

	Clear();

	result.status = GameSceneLoadStatus::Succeeded;

	LOG_INFO("Scene: Level '" + desc.name + "' loaded");
	return result;
}

bool GameScene::AppendImportedSceneData(ImportedSceneData&& importedSceneData)
{
	if (!importedSceneData.HasMeshes())
	{
		return false;
	}

	if (!importedSceneData.materials.empty())
	{
		m_textures.AppendMaterialTextureReferences(importedSceneData.materials);
	}

	const MaterialHandle materialBaseHandle =
	    importedSceneData.materials.empty() ? MaterialHandle::Invalid() : m_materials.AppendMaterials(std::move(importedSceneData.materials));

	std::vector<std::unique_ptr<MeshComponent>> importedMeshes;
	importedMeshes.reserve(importedSceneData.meshes.size());
	for (std::size_t meshIndex = 0; meshIndex < importedSceneData.meshes.size(); ++meshIndex)
	{
		auto mesh = std::make_unique<ImportedMesh>(std::move(importedSceneData.meshes[meshIndex]));
		const MaterialHandle localMaterialHandle =
		    meshIndex < importedSceneData.materialHandles.size() ? importedSceneData.materialHandles[meshIndex] : MaterialHandle::Invalid();
		const Transform importedTransform =
		    meshIndex < importedSceneData.transforms.size() ? importedSceneData.transforms[meshIndex] : Transform();
		const MaterialHandle materialHandle =
		    localMaterialHandle.IsValid() && materialBaseHandle.IsValid()
		        ? MaterialHandle(materialBaseHandle.GetIndex() + localMaterialHandle.GetIndex())
		        : m_materials.GetOrCreateDefaultMaterialHandle();
		importedMeshes.push_back(std::make_unique<MeshComponent>(std::move(mesh), importedTransform, materialHandle));
	}

	m_meshes.AppendMeshComponents(std::move(importedMeshes));

	LOG_INFO(
	    "Scene: Loaded " + std::to_string(m_meshes.GetMeshCount()) + " meshes, " + std::to_string(m_materials.GetMaterialCount()) +
	    " materials");

	return true;
}

void GameScene::Clear()
{
	m_lighting.Reset();
	m_materials.Reset();
	m_meshes.Reset();
	m_textures.Reset();
}
