#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCameras.h"
#include "GameFramework/Public/Scene/Lighting/SceneLighting.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterials.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterialVariants.h"
#include "GameFramework/Public/Scene/GameSceneController.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshes.h"
#include "GameFramework/Public/Scene/Skeletons/SceneSkeletons.h"
#include "GameFramework/Public/Scene/Sky/SceneSky.h"
#include "GameFramework/Public/Scene/GameSceneSnapshot.h"
#include "GameFramework/Public/Scene/Textures/SceneTextures.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ECS
{
	class SceneWorld;
}

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
	SceneSky& GetSky() noexcept { return m_sky; }
	const SceneSky& GetSky() const noexcept { return m_sky; }

	GameSceneLoadResult LoadLevel(const LevelAsset& level);
	GameSceneLoadResult LoadLevel(const LevelDesc& desc);
	void Update(float deltaSeconds);
	bool AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload);
	GameSceneSnapshot CaptureSnapshot() const;
	std::string_view GetActiveLevelName() const noexcept { return m_activeLevelName; }
	void RegisterController(std::unique_ptr<GameSceneController>&& controller);

	void Clear();
	bool IsEntityAlive(EntityId entity) const noexcept;
	bool DestroyEntity(EntityId entity) noexcept;

	SceneMaterials& GetMaterials() noexcept { return m_materials; }
	const SceneMaterials& GetMaterials() const noexcept { return m_materials; }
	SceneMaterialVariants& GetMaterialVariants() noexcept { return m_materialVariants; }
	const SceneMaterialVariants& GetMaterialVariants() const noexcept { return m_materialVariants; }
	SceneMeshes& GetMeshes() noexcept { return m_meshes; }
	const SceneMeshes& GetMeshes() const noexcept { return m_meshes; }
	SceneTextures& GetTextures() noexcept { return m_textures; }
	const SceneTextures& GetTextures() const noexcept { return m_textures; }
	SceneSkeletons& GetSkeletons() noexcept { return m_skeletons; }
	const SceneSkeletons& GetSkeletons() const noexcept { return m_skeletons; }
  private:
	friend class SceneCameraView;
	friend class SceneCameras;
	friend class SceneLighting;
	friend class SceneMeshes;
	friend class SceneMeshView;

	std::unique_ptr<ECS::SceneWorld> m_world;
	SceneCameras m_cameras;
	SceneLighting m_lighting;
	SceneMaterials m_materials;
	SceneMaterialVariants m_materialVariants;
	SceneMeshes m_meshes;
	SceneSkeletons m_skeletons;
	SceneSky m_sky;
	SceneTextures m_textures;
	std::string m_activeLevelName;
	LevelDesc m_activeLevelDesc;
	std::vector<std::unique_ptr<GameSceneController>> m_controllers;
};
