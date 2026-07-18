#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCameras.h"
#include "GameFramework/Public/Scene/Lighting/SceneLighting.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterials.h"
#include "GameFramework/Public/Scene/Materials/SceneMaterialVariants.h"
#include "GameFramework/Public/World/GameWorldController.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshes.h"
#include "GameFramework/Public/Scene/Skeletons/SceneSkeletons.h"
#include "GameFramework/Public/Scene/Sky/SceneSky.h"
#include "GameFramework/Public/World/GameWorldSnapshot.h"
#include "GameFramework/Public/World/WorldChange.h"
#include "GameFramework/Public/World/WorldReadView.h"
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
	class GameWorldState;
}

class LevelAsset;
struct SceneAssetPayload;

enum class GameWorldLoadStatus : std::uint8_t
{
	Succeeded = 0,
	Failed
};

struct SPARKLE_ENGINE_API GameWorldLoadResult
{
	GameWorldLoadStatus status = GameWorldLoadStatus::Failed;
	std::string errorMessage;

	bool Succeeded() const noexcept { return status == GameWorldLoadStatus::Succeeded; }
};

class SPARKLE_ENGINE_API GameWorld final
{
  public:
	GameWorld();
	~GameWorld() noexcept;

	GameWorld(const GameWorld&) = delete;
	GameWorld& operator=(const GameWorld&) = delete;
	GameWorld(GameWorld&&) = delete;
	GameWorld& operator=(GameWorld&&) = delete;

	SceneCameras& GetCameras() noexcept { return m_cameras; }
	const SceneCameras& GetCameras() const noexcept { return m_cameras; }
	SceneLighting& GetLighting() noexcept { return m_lighting; }
	const SceneLighting& GetLighting() const noexcept { return m_lighting; }
	SceneSky& GetSky() noexcept { return m_sky; }
	const SceneSky& GetSky() const noexcept { return m_sky; }

	GameWorldLoadResult LoadLevel(const LevelAsset& level);
	GameWorldLoadResult LoadLevel(const LevelDesc& desc);
	void Update(float deltaSeconds);
	bool AppendSceneAssetPayload(SceneAssetPayload&& sceneAssetPayload);
	GameWorldSnapshot CaptureSnapshot() const;
	WorldReadView AcquireReadView() const noexcept;
	WorldChangeBatch ReadChanges(const WorldChangeCursor& cursor) const;
	bool AcknowledgeChanges(WorldChangeCursor& cursor, WorldSequence sequence) const noexcept;
	std::string_view GetActiveLevelName() const noexcept { return m_activeLevelName; }
	void RegisterController(std::unique_ptr<GameWorldController>&& controller);

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
	friend class SceneSky;
	void CommitWorldChanges();

	std::unique_ptr<ECS::GameWorldState> m_state;
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
	std::vector<std::unique_ptr<GameWorldController>> m_controllers;
};
