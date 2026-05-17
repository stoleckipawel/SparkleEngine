#pragma once

#include "GameFramework/Public/Assets/SceneAssetPayload.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCamera.h"
#include "GameFramework/Public/Scene/Lighting/SceneLighting.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterials.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshes.h"
#include "GameFramework/Public/Scene/GameSceneSnapshot.h"
#include "GameFramework/Public/Scene/Textures/SceneTextures.h"

#include <cstdint>
#include <string>

class LevelAsset;

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
	SceneCamera m_sceneCamera;
	SceneLighting m_lighting;
	SceneMaterials m_materials;
	SceneMeshes m_meshes;
	SceneTextures m_textures;
};
