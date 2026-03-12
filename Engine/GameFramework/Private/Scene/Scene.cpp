#include "PCH.h"
#include "Scene.h"

#include "Scene/Mesh.h"
#include "Scene/ImportedMesh.h"
#include "Camera/GameCamera.h"
#include "Assets/AssetSystem.h"
#include "Assets/GltfLoader.h"
#include "Level/Level.h"
#include "Level/LevelDesc.h"
#include "Core/Public/Diagnostics/Log.h"
#include "Scene/MeshFactory.h"

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
	LoadMeshRequests(desc, assetSystem);

	m_currentLevelName = std::string(level.GetName());

	LOG_INFO("Scene: Level '" + m_currentLevelName + "' loaded");
}

void Scene::LoadMeshRequests(const LevelDesc& desc, AssetSystem& assetSystem)
{
	for (const auto& request : desc.meshRequests)
	{
		switch (request.source)
		{
			case AssetSource::Imported:
				LoadImportedMeshRequest(request, assetSystem);
				break;
			case AssetSource::Procedural:
				LoadProceduralMeshRequest(request);
				break;
			default:
				LOG_FATAL("Scene: Unhandled AssetSource in LoadMeshRequests");
				break;
		}
	}
}

void Scene::LoadImportedMeshRequest(const MeshRequest& request, AssetSystem& assetSystem)
{
	auto resolved = assetSystem.ResolvePath(request.assetPath, request.assetType);
	if (resolved)
	{
		AppendGltf(*resolved);
		return;
	}

	LOG_WARNING("Scene: Asset not found — " + request.assetPath.string());
}

void Scene::LoadProceduralMeshRequest(const MeshRequest& request)
{
	AppendProceduralMeshes(request.procedural);
}

void Scene::AppendProceduralMeshes(const PrimitiveRequest& request)
{
	MeshFactory factory;
	factory.AppendShapes(request.shape, request.count, request.center, request.extents, request.seed);

	std::vector<std::unique_ptr<Mesh>> meshes = std::move(factory).TakeMeshes();
	AddMeshes(std::move(meshes));
}

void Scene::Clear()
{
	m_meshes.clear();
	m_loadedMaterials.clear();
	m_currentLevelName.clear();
}

// =============================================================================
// Asset Loading
// =============================================================================

bool Scene::LoadGltf(const std::filesystem::path& filePath)
{
	Clear();
	return AppendGltf(filePath);
}

bool Scene::AppendGltf(const std::filesystem::path& filePath)
{
	LOG_INFO("Scene: Loading glTF from " + filePath.string());

	GltfLoader::LoadResult result = GltfLoader::Load(filePath);

	if (!result.IsValid())
	{
		LOG_ERROR("Scene: Failed to load glTF — " + result.errorMessage);
		return false;
	}

	const std::size_t materialOffset = m_loadedMaterials.size();

	// Store materials from the glTF file
	if (!result.materials.empty())
	{
		m_loadedMaterials.reserve(m_loadedMaterials.size() + result.materials.size());
		for (auto& material : result.materials)
		{
			m_loadedMaterials.push_back(std::move(material));
		}
	}

	// Create ImportedMesh for each primitive
	m_meshes.reserve(m_meshes.size() + result.meshes.size());
	for (std::size_t i = 0; i < result.meshes.size(); ++i)
	{
		auto mesh = std::make_unique<ImportedMesh>(std::move(result.meshes[i]), result.transforms[i]);

		// Map glTF material index to scene-level material
		if (i < result.materialIndices.size())
		{
			mesh->SetMaterialId(static_cast<uint32_t>(materialOffset) + result.materialIndices[i]);
		}

		m_meshes.push_back(std::move(mesh));
	}

	LOG_INFO("Scene: Loaded " + std::to_string(m_meshes.size()) + " meshes, " + std::to_string(m_loadedMaterials.size()) + " materials");

	return true;
}

// =============================================================================
// Mesh Management
// =============================================================================

void Scene::AddMeshes(std::vector<std::unique_ptr<Mesh>> meshes)
{
	m_meshes.reserve(m_meshes.size() + meshes.size());
	for (auto& mesh : meshes)
	{
		m_meshes.push_back(std::move(mesh));
	}
}
