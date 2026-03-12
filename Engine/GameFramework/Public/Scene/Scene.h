// ============================================================================
// Scene.h
// ----------------------------------------------------------------------------
// Container for gameplay objects (camera, meshes, etc.).
//
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
class AssetSystem;
struct LevelDesc;
struct MeshRequest;
struct PrimitiveRequest;

class SPARKLE_ENGINE_API Scene final
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	Scene();
	~Scene() noexcept;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(Scene&&) = delete;

	// ========================================================================
	// Camera
	// ========================================================================

	GameCamera& GetCamera() noexcept;
	const GameCamera& GetCamera() const noexcept;

	// ========================================================================
	// Level Loading
	// ========================================================================

	/// Loads a level into the scene, replacing all current content.
	void LoadLevel(const Level& level, AssetSystem& assetSystem);

	/// Clears all scene content (meshes, materials, level state).
	void Clear();

	/// Returns the name of the currently loaded level (empty if none).
	const std::string& GetCurrentLevelName() const noexcept { return m_currentLevelName; }

	// ========================================================================
	// Asset Loading
	// ========================================================================

	/// Loads a glTF file and replaces the current scene contents.
	/// Clears any procedural primitives. Materials are stored and
	/// accessible via GetLoadedMaterials().
	bool LoadGltf(const std::filesystem::path& filePath);

	/// Returns materials loaded from the last glTF import.
	const std::vector<MaterialDesc>& GetLoadedMaterials() const noexcept { return m_loadedMaterials; }

	// ========================================================================
	// Mesh Management
	// ========================================================================

	/// Takes ownership of externally-created meshes (e.g., from MeshFactory or glTF).
	void AddMeshes(std::vector<std::unique_ptr<Mesh>> meshes);

	// ========================================================================
	// Accessors
	// ========================================================================

	const std::vector<std::unique_ptr<Mesh>>& GetMeshes() const noexcept { return m_meshes; }
	bool HasMeshes() const noexcept { return !m_meshes.empty(); }

  private:
	void LoadMeshRequests(const LevelDesc& desc, AssetSystem& assetSystem);
	void LoadImportedMeshRequest(const MeshRequest& request, AssetSystem& assetSystem);
	void LoadProceduralMeshRequest(const MeshRequest& request);
	void AppendProceduralMeshes(const PrimitiveRequest& request);
	bool AppendGltf(const std::filesystem::path& filePath);

	// ------------------------------------------------------------------------
	// Owned Objects
	// ------------------------------------------------------------------------

	std::unique_ptr<GameCamera> m_camera;

	// All meshes in the scene (procedural, imported, etc.)
	std::vector<std::unique_ptr<Mesh>> m_meshes;
	std::vector<MaterialDesc> m_loadedMaterials;

	// ------------------------------------------------------------------------
	// State
	// ------------------------------------------------------------------------

	std::string m_currentLevelName;
};
