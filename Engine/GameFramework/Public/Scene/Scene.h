#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Assets/MaterialDesc.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Mesh;
class GameCamera;
class Level;
class LevelRegistry;
class AssetSystem;
struct LevelDesc;
struct ImportedMeshRequest;

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

	void LoadLevel(const Level& level, AssetSystem& assetSystem);
	void LoadLevelOrDefault(const LevelRegistry& levelRegistry, std::string_view levelName, AssetSystem& assetSystem);

	void Clear();

	const std::string& GetCurrentLevelName() const noexcept { return m_currentLevelName; }

	bool LoadGltf(const std::filesystem::path& filePath);

	const std::vector<MaterialDesc>& GetLoadedMaterials() const noexcept { return m_loadedMaterials; }

	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const noexcept { return m_meshes; }
	bool HasMeshes() const noexcept { return !m_meshes.empty(); }

  private:
	void LoadImportedMeshRequests(const LevelDesc& desc, AssetSystem& assetSystem);
	void LoadImportedMeshRequest(const ImportedMeshRequest& request, AssetSystem& assetSystem);
	bool AppendGltf(const std::filesystem::path& filePath);

	std::unique_ptr<GameCamera> m_camera;

	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<MaterialDesc> m_loadedMaterials;

	std::string m_currentLevelName;
};
