#include "PCH.h"
#include "Scene.h"

#include "FileSystemUtils.h"
#include "Scene/Mesh.h"
#include "Scene/ImportedMesh.h"
#include "Camera/GameCamera.h"
#include "Assets/GltfLoader.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Core/Public/Diagnostics/Log.h"

Scene::Scene() : m_gameCamera(std::make_unique<GameCamera>()) {}

Scene::~Scene() noexcept = default;

GameCamera& Scene::GetCamera() noexcept
{
	return *m_gameCamera;
}

const GameCamera& Scene::GetCamera() const noexcept
{
	return *m_gameCamera;
}

SceneLoadResult Scene::LoadLevel(const Level& level)
{
	SceneLoadResult result;
	const std::string levelName(level.GetName());

	LOG_INFO("Scene: Loading level '" + levelName + "'");

	Clear();

	LevelDesc desc = level.BuildDescription();
	if (!LoadImportedMeshRequests(desc, result.errorMessage))
	{
		Clear();
		LOG_ERROR(
		    "Scene: Failed to load level '" + levelName + "'" +
		    (result.errorMessage.empty() ? std::string() : " - " + result.errorMessage));
		return result;
	}

	m_camera = desc.initialCamera;
	result.status = SceneLoadStatus::Succeeded;

	LOG_INFO("Scene: Level '" + levelName + "' loaded");
	return result;
}

bool Scene::LoadImportedMeshRequests(const LevelDesc& desc, std::string& errorMessage)
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

bool Scene::LoadImportedMeshRequest(const ImportedMeshRequest& request, std::string& errorMessage)
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

void Scene::Clear()
{
	m_meshes.clear();
	m_loadedMaterials.clear();
	m_camera = {};
}

bool Scene::LoadGltf(const std::filesystem::path& assetPath)
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

bool Scene::AppendResolvedGltf(const std::filesystem::path& resolvedPath)
{
	LOG_INFO("Scene: Loading glTF from " + resolvedPath.string());

	GltfLoader::LoadResult result = GltfLoader::Load(resolvedPath);

	if (!result.IsValid())
	{
		LOG_ERROR("Scene: Failed to load glTF — " + result.errorMessage);
		return false;
	}

	const std::size_t materialOffset = m_loadedMaterials.size();

	if (!result.materials.empty())
	{
		m_loadedMaterials.reserve(m_loadedMaterials.size() + result.materials.size());
		for (auto& material : result.materials)
		{
			m_loadedMaterials.push_back(std::move(material));
		}
	}

	m_meshes.reserve(m_meshes.size() + result.meshes.size());
	for (std::size_t i = 0; i < result.meshes.size(); ++i)
	{
		auto mesh = std::make_unique<ImportedMesh>(std::move(result.meshes[i]), result.transforms[i]);

		if (i < result.materialIndices.size())
		{
			mesh->SetMaterialId(static_cast<uint32_t>(materialOffset) + result.materialIndices[i]);
		}

		m_meshes.push_back(std::move(mesh));
	}

	LOG_INFO("Scene: Loaded " + std::to_string(m_meshes.size()) + " meshes, " + std::to_string(m_loadedMaterials.size()) + " materials");

	return true;
}
