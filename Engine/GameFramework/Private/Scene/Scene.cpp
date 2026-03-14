#include "PCH.h"
#include "Scene.h"

#include "Scene/Mesh.h"
#include "Scene/ImportedMesh.h"
#include "Camera/GameCamera.h"
#include "Assets/AssetSystem.h"
#include "Assets/GltfLoader.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Level/LevelRegistry.h"
#include "Core/Public/Diagnostics/Log.h"

Scene::Scene() : m_camera(std::make_unique<GameCamera>()) {}

Scene::~Scene() noexcept = default;

GameCamera& Scene::GetCamera() noexcept
{
	return *m_camera;
}

const GameCamera& Scene::GetCamera() const noexcept
{
	return *m_camera;
}

void Scene::LoadLevel(const Level& level, AssetSystem& assetSystem)
{
	LOG_INFO("Scene: Loading level '" + std::string(level.GetName()) + "'");

	Clear();

	LevelDesc desc = level.BuildDescription();
	LoadImportedMeshRequests(desc, assetSystem);

	m_currentLevelName = std::string(level.GetName());

	LOG_INFO("Scene: Level '" + m_currentLevelName + "' loaded");
}

void Scene::LoadLevelOrDefault(const LevelRegistry& levelRegistry, std::string_view levelName, AssetSystem& assetSystem)
{
	if (auto* level = levelRegistry.FindLevelOrDefault(levelName))
	{
		LoadLevel(*level, assetSystem);
	}
}

void Scene::LoadImportedMeshRequests(const LevelDesc& desc, AssetSystem& assetSystem)
{
	for (const auto& request : desc.importedMeshRequests)
	{
		LoadImportedMeshRequest(request, assetSystem);
	}
}

void Scene::LoadImportedMeshRequest(const ImportedMeshRequest& request, AssetSystem& assetSystem)
{
	auto resolved = assetSystem.ResolvePath(request.assetPath, AssetType::Mesh);
	if (resolved)
	{
		AppendResolvedGltf(*resolved);
		return;
	}

	LOG_WARNING("Scene: Asset not found — " + request.assetPath.string());
}

void Scene::Clear()
{
	m_meshes.clear();
	m_loadedMaterials.clear();
	m_currentLevelName.clear();
}

bool Scene::LoadGltf(const std::filesystem::path& assetPath, AssetSystem& assetSystem)
{
	Clear();

	auto resolvedPath = assetSystem.ResolvePath(assetPath, AssetType::Mesh);
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
