#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Mesh;
class GameCamera;
class Level;
struct ImportedMeshRequest;

enum class SceneLoadStatus : std::uint8_t
{
	Succeeded = 0,
	Failed
};

struct SPARKLE_ENGINE_API SceneLoadResult
{
	SceneLoadStatus status = SceneLoadStatus::Failed;
	std::string errorMessage;

	bool Succeeded() const noexcept { return status == SceneLoadStatus::Succeeded; }
};

class SPARKLE_ENGINE_API Scene final
{
  public:
	Scene();
	~Scene() noexcept;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(Scene&&) = delete;

	GameCamera& GetCamera() noexcept;
	const GameCamera& GetCamera() const noexcept;

	SceneLoadResult LoadLevel(const Level& level);

	void Clear();

	const LevelCameraDesc& GetCurrentLevelInitialCamera() const noexcept { return m_camera; }

	bool LoadGltf(const std::filesystem::path& assetPath);

	const std::vector<MaterialDesc>& GetLoadedMaterials() const noexcept { return m_loadedMaterials; }

	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const noexcept { return m_meshes; }
	bool HasMeshes() const noexcept { return !m_meshes.empty(); }

  private:
	bool LoadImportedMeshRequests(const LevelDesc& desc, std::string& errorMessage);
	bool LoadImportedMeshRequest(const ImportedMeshRequest& request, std::string& errorMessage);
	bool AppendResolvedGltf(const std::filesystem::path& resolvedPath);

	std::unique_ptr<GameCamera> m_gameCamera;

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<MaterialDesc> m_loadedMaterials;

	LevelCameraDesc m_camera;
};
