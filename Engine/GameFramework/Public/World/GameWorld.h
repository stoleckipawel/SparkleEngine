#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/SceneCameras.h"
#include "GameFramework/Public/Scene/Lighting/SceneLighting.h"
#include "GameFramework/Public/Scene/Materials/MaterialVariant.h"
#include "GameFramework/Public/World/GameWorldController.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshes.h"
#include "GameFramework/Public/Scene/Sky/SceneSky.h"
#include "GameFramework/Public/World/GameWorldSnapshot.h"
#include "GameFramework/Public/World/WorldChange.h"
#include "GameFramework/Public/World/WorldReadView.h"
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
namespace Assets
{
	struct SceneLoadPackage;
}

struct GameWorldResourceStores;

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

	void Update(float deltaSeconds);
	GameWorldSnapshot CaptureSnapshot() const;
	WorldReadView AcquireReadView() const noexcept;
	WorldChangeBatch ReadChanges(const WorldChangeCursor& cursor) const;
	bool AcknowledgeChanges(WorldChangeCursor& cursor, WorldSequence sequence) const noexcept;
	std::string_view GetActiveLevelName() const noexcept { return m_activeLevelName; }
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	void RegisterController(std::unique_ptr<GameWorldController>&& controller);
	std::size_t GetMaterialVariantCount() const noexcept;
	std::string_view GetMaterialVariantName(std::size_t index) const noexcept;
	MaterialVariantIndex GetActiveMaterialVariant() const noexcept;
	bool ApplyMaterialVariant(MaterialVariantIndex index);

	bool IsEntityAlive(EntityId entity) const noexcept;
	bool DestroyEntity(EntityId entity) noexcept;

	SceneMeshes& GetMeshes() noexcept { return m_meshes; }
	const SceneMeshes& GetMeshes() const noexcept { return m_meshes; }
  private:
	friend class LevelManager;
	friend class SceneCameraView;
	friend class SceneCameras;
	friend class SceneLighting;
	friend class SceneMeshes;
	friend class SceneMeshView;
	friend class SceneSky;
	void CommitWorldChanges();
	void InitializeStagedLevel(const LevelDesc& desc);
	bool CommitSceneLoadPackage(Assets::SceneLoadPackage&& package, std::string& errorMessage);
	void FinalizeSceneLoadCommit();

	std::unique_ptr<ECS::GameWorldState> m_state;
	std::unique_ptr<GameWorldResourceStores> m_resources;
	SceneCameras m_cameras;
	SceneLighting m_lighting;
	SceneMeshes m_meshes;
	SceneSky m_sky;
	std::string m_activeLevelName;
	LevelDesc m_activeLevelDesc;
	std::vector<std::unique_ptr<GameWorldController>> m_controllers;
	std::uint64_t m_generation = 1;
};
