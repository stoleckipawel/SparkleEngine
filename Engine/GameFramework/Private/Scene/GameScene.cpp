#include "PCH.h"
#include "GameScene.h"

#include "FileSystemUtils.h"
#include "Scene/Mesh.h"
#include "Scene/ImportedMesh.h"
#include "Camera/CameraComponent.h"
#include "Assets/GltfLoader.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Core/Public/Diagnostics/Log.h"

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

	if (!LoadImportedMeshRequests(desc, result.errorMessage))
	{
		Clear();
		LOG_ERROR(
		    "Scene: Failed to load level '" + desc.name + "'" +
		    (result.errorMessage.empty() ? std::string() : " - " + result.errorMessage));
		return result;
	}

	result.status = GameSceneLoadStatus::Succeeded;

	LOG_INFO("Scene: Level '" + desc.name + "' loaded");
	return result;
}

bool GameScene::LoadImportedMeshRequests(const LevelDesc& desc, std::string& errorMessage)
{
	for (const auto& request : desc.importedMeshRequests)
	{
		if (!LoadImportedMeshRequest(request, errorMessage))
		{
			return false;
		}
	}

	return true;
}

bool GameScene::LoadImportedMeshRequest(const ImportedMeshRequest& request, std::string& errorMessage)
{
	auto resolved = Filesystem::ResolveAssetPath(request.assetPath, AssetType::Mesh);
	if (resolved)
	{
		if (AppendResolvedGltf(*resolved))
		{
			return true;
		}

		errorMessage = "Failed to load mesh asset '" + resolved->string() + "'";
		return false;
	}

	errorMessage = "Mesh asset not found '" + request.assetPath.string() + "'";
	LOG_WARNING("Scene: Asset not found — " + request.assetPath.string());
	return false;
}

void GameScene::Clear()
{
	m_lighting.Reset();
	m_materials.Reset();
	m_meshes.Reset();
	m_textures.Reset();
}

bool GameScene::LoadGltf(const std::filesystem::path& assetPath)
{
	Clear();

	auto resolvedPath = Filesystem::ResolveAssetPath(assetPath, AssetType::Mesh);
	if (!resolvedPath)
	{
		LOG_WARNING("Scene: Asset not found — " + assetPath.string());
		return false;
	}

	return AppendResolvedGltf(*resolvedPath);
}

bool GameScene::AppendResolvedGltf(const std::filesystem::path& resolvedPath)
{
	LOG_INFO("Scene: Loading glTF from " + resolvedPath.string());

	GltfLoader::LoadResult result = GltfLoader::Load(resolvedPath);

	if (!result.IsValid())
	{
		LOG_ERROR("Scene: Failed to load glTF — " + result.errorMessage);
		return false;
	}

	if (!result.materials.empty())
	{
		m_textures.AppendMaterialTextureReferences(result.materials);
	}

	const std::uint32_t materialBaseId = m_materials.AppendMaterials(std::move(result.materials));

	std::vector<std::unique_ptr<Mesh>> importedMeshes;
	importedMeshes.reserve(result.meshes.size());
	for (std::size_t i = 0; i < result.meshes.size(); ++i)
	{
		auto mesh = std::make_unique<ImportedMesh>(std::move(result.meshes[i]), result.transforms[i]);
		const std::uint32_t localMaterialId =
		    i < result.materialIndices.size() ? result.materialIndices[i] : 0;
		mesh->SetMaterialId(materialBaseId + localMaterialId);

		importedMeshes.push_back(std::move(mesh));
	}

	m_meshes.AppendMeshes(std::move(importedMeshes));

	LOG_INFO(
	    "Scene: Loaded " + std::to_string(m_meshes.GetMeshCount()) + " meshes, " + std::to_string(m_materials.GetMaterialCount()) +
	    " materials");

	return true;
}
