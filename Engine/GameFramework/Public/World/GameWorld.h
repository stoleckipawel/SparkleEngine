#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"
#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/Scene/Materials/MaterialVariant.h"
#include "GameFramework/Public/World/WorldChange.h"
#include "GameFramework/Public/World/WorldReadView.h"
#include "GameFramework/Public/World/EntityId.h"
#include "GameFramework/Public/Rendering/RenderInputFrame.h"
#include "GameFramework/Public/World/WorldEditCommand.h"
#include "GameFramework/Public/World/WorldMaterialVariantView.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ECS
{
	class GameWorldState;
	class RenderInputExtractor;
}
namespace Assets
{
	struct SceneLoadPackage;
}

struct GameWorldResourceStores;
class WorldEditCommandQueue;
class TaskExecutor;

class SPARKLE_ENGINE_API GameWorld final
{
  public:
	explicit GameWorld(TaskExecutor& taskExecutor);
	~GameWorld() noexcept;

	GameWorld(const GameWorld&) = delete;
	GameWorld& operator=(const GameWorld&) = delete;
	GameWorld(GameWorld&&) = delete;
	GameWorld& operator=(GameWorld&&) = delete;

	void Update(float deltaSeconds);
	RenderInputFrame ExtractRenderInput(RenderFrameMetadata metadata);
	WorldReadView AcquireReadView() const noexcept;
	WorldChangeBatch ReadChanges(const WorldChangeCursor& cursor) const;
	bool AcknowledgeChanges(WorldChangeCursor& cursor, WorldSequence sequence) const noexcept;
	std::string_view GetActiveLevelName() const noexcept { return m_activeLevelName; }
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	void PublishCameraInputIntent(const CameraInputIntent& intent) noexcept;
	void EnableOscillatingMeshMotion(bool enabled = true);
	std::size_t GetMaterialVariantCount() const noexcept;
	std::string_view GetMaterialVariantName(std::size_t index) const noexcept;
	MaterialVariantIndex GetActiveMaterialVariant() const noexcept;
	bool ApplyMaterialVariant(MaterialVariantIndex index);
	WorldMaterialVariantView CaptureMaterialVariants() const;
	WorldEditResult SubmitEdit(WorldEditCommand command, std::uint64_t expectedGeneration);

	bool IsEntityAlive(EntityId entity) const noexcept;
	bool DestroyEntity(EntityId entity) noexcept;

  private:
	friend class LevelSession;
	void CommitWorldChanges();
	void InitializeStagedLevel(const LevelDesc& desc);
	void CommitSceneLoadPackage(Assets::SceneLoadPackage&& package);
	void FinalizeSceneLoadCommit();

	std::unique_ptr<ECS::GameWorldState> m_state;
	std::unique_ptr<GameWorldResourceStores> m_resources;
	TaskExecutor& m_taskExecutor;
	std::string m_activeLevelName;
	LevelDesc m_activeLevelDesc;
	CameraInputIntent m_cameraInputIntent;
	bool m_oscillatingMeshMotionEnabled = false;
	std::uint64_t m_generation = 1;
	std::unique_ptr<WorldEditCommandQueue> m_editCommands;
	std::unique_ptr<ECS::RenderInputExtractor> m_renderInputExtractor;
};
