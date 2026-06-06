#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCameras.h"
#include "GameFramework/Public/Scene/Lighting/SceneLighting.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterials.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshes.h"
#include "GameFramework/Public/Scene/GameSceneSnapshot.h"
#include "GameFramework/Public/Scene/Textures/SceneTextures.h"

#include <cstddef>
#include <cstdint>
#include <string>

class LevelAsset;
struct SceneAssetPayload;

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

	SceneCameras& GetCameras() noexcept { return m_cameras; }
	const SceneCameras& GetCameras() const noexcept { return m_cameras; }
	SceneLighting& GetLighting() noexcept { return m_lighting; }
	const SceneLighting& GetLighting() const noexcept { return m_lighting; }

	GameSceneLoadResult LoadLevel(const LevelAsset& level);
	GameSceneLoadResult LoadLevel(const LevelDesc& desc);
	bool AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload);
	GameSceneSnapshot CaptureSnapshot() const;

	void Clear();

	SceneMaterials& GetMaterials() noexcept { return m_materials; }
	const SceneMaterials& GetMaterials() const noexcept { return m_materials; }
	SceneMeshes& GetMeshes() noexcept { return m_meshes; }
	const SceneMeshes& GetMeshes() const noexcept { return m_meshes; }
	SceneTextures& GetTextures() noexcept { return m_textures; }
	const SceneTextures& GetTextures() const noexcept { return m_textures; }

  private:
	SceneCameras m_cameras;
	SceneLighting m_lighting;
	SceneMaterials m_materials;
	SceneMeshes m_meshes;
	SceneTextures m_textures;
};
