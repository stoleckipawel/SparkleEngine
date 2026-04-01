#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCamera.h"
#include "GameFramework/Public/Scene/Lighting/SceneLighting.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterials.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshes.h"
#include "GameFramework/Public/Scene/Textures/SceneTextures.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Mesh;
class LevelAsset;
struct ImportedMeshRequest;
struct SceneImportResult;

enum class GameSceneLoadStatus : std::uint8_t
{
	Succeeded = 0,
	Failed
};

struct SPARKLE_ENGINE_API GameSceneLoadResult
{
	GameSceneLoadStatus status = GameSceneLoadStatus::Failed;
	std::string errorMessage;

	bool Succeeded() const noexcept { return status == GameSceneLoadStatus::Succeeded; }
};

class SPARKLE_ENGINE_API GameScene final
{
  public:
	GameScene();
	~GameScene() noexcept;

	GameScene(const GameScene&) = delete;
	GameScene& operator=(const GameScene&) = delete;
	GameScene(GameScene&&) = delete;
	GameScene& operator=(GameScene&&) = delete;

	SceneCamera& GetSceneCamera() noexcept { return m_sceneCamera; }
	const SceneCamera& GetSceneCamera() const noexcept { return m_sceneCamera; }
	SceneLighting& GetLighting() noexcept { return m_lighting; }
	const SceneLighting& GetLighting() const noexcept { return m_lighting; }

	GameSceneLoadResult LoadLevel(const LevelAsset& level);
	GameSceneLoadResult LoadLevel(const LevelDesc& desc);

	void Clear();

	bool LoadGltf(const std::filesystem::path& assetPath);

	SceneMaterials& GetMaterials() noexcept { return m_materials; }
	const SceneMaterials& GetMaterials() const noexcept { return m_materials; }
	SceneMeshes& GetMeshes() noexcept { return m_meshes; }
	const SceneMeshes& GetMeshes() const noexcept { return m_meshes; }
	SceneTextures& GetTextures() noexcept { return m_textures; }
	const SceneTextures& GetTextures() const noexcept { return m_textures; }

  private:
	bool LoadImportedMeshRequests(const LevelDesc& desc, std::string& errorMessage);
	bool LoadImportedMeshRequest(const ImportedMeshRequest& request, std::string& errorMessage);
	bool AppendResolvedImportedAsset(const std::filesystem::path& resolvedPath);
	bool AppendImportedScene(SceneImportResult&& result);

	SceneCamera m_sceneCamera;
	SceneLighting m_lighting;
	SceneMaterials m_materials;
	SceneMeshes m_meshes;
	SceneTextures m_textures;
};
