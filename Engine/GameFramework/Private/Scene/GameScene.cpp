#include "PCH.h"
#include "GameScene.h"

#include "FileSystemUtils.h"
#include "Scene/MeshComponent.h"
#include "Scene/Mesh.h"
#include "Scene/ImportedMesh.h"
#include "Camera/CameraComponent.h"
#include "Assets/Import/SceneImportResult.h"
#include "Assets/Import/SceneImporter.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Core/Public/Diagnostics/Log.h"

#include <format>

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
		if (AppendResolvedImportedAsset(*resolved))
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

	return AppendResolvedImportedAsset(*resolvedPath);
}

bool GameScene::AppendResolvedImportedAsset(const std::filesystem::path& resolvedPath)
{
	LOG_INFO("Scene: Loading imported asset from " + resolvedPath.string());

	SceneImportResult result = SceneImporter::Load(resolvedPath);

	if (!result.IsValid())
	{
		LOG_ERROR("Scene: Failed to load imported asset — " + result.errorMessage);
		return false;
	}

	LOG_INFO(std::format(
	    "Scene: Import summary [{}] '{}' took {:.2f} ms — {} meshes, {} materials, {} warnings, {} unique textures, {} duplicate texture refs, {} material duplicates removed, {:.2f} MiB mesh data",
	    result.stats.importerName,
	    result.stats.sourcePath.filename().string(),
	    result.stats.importDurationMs,
	    result.meshes.size(),
	    result.materials.size(),
	    result.warnings.size(),
	    result.stats.uniqueTexturePathCount,
	    result.stats.duplicateTexturePathCount,
	    result.stats.deduplicatedMaterialCount,
	    static_cast<double>(result.stats.estimatedMeshBytes) / (1024.0 * 1024.0)));

	for (const SceneImportWarning& warning : result.warnings)
	{
		LOG_WARNING("Scene: Import warning — " + warning.message);
	}

	return AppendImportedScene(std::move(result));
}

bool GameScene::AppendImportedScene(SceneImportResult&& result)
{

	if (!result.materials.empty())
	{
		m_textures.AppendMaterialTextureReferences(result.materials);
	}

	const MaterialHandle materialBaseHandle = m_materials.AppendMaterials(std::move(result.materials));

	std::vector<std::unique_ptr<MeshComponent>> importedMeshes;
	importedMeshes.reserve(result.meshes.size());
	for (std::size_t i = 0; i < result.meshes.size(); ++i)
	{
		auto mesh = std::make_unique<ImportedMesh>(std::move(result.meshes[i]));
		const std::uint32_t localMaterialOffset =
		    i < result.materialOffsets.size() ? result.materialOffsets[i] : 0;
		const Transform importedTransform =
		    i < result.transforms.size() ? result.transforms[i] : Transform();
		const MaterialHandle materialHandle(materialBaseHandle.GetIndex() + localMaterialOffset);
		importedMeshes.push_back(
		    std::make_unique<MeshComponent>(std::move(mesh), importedTransform, materialHandle));
	}

	m_meshes.AppendMeshComponents(std::move(importedMeshes));

	LOG_INFO(
	    "Scene: Loaded " + std::to_string(m_meshes.GetMeshCount()) + " meshes, " + std::to_string(m_materials.GetMaterialCount()) +
	    " materials");

	return true;
}
