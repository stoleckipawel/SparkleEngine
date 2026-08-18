#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstdint>
#include <memory>
#include <span>

using WorldSequence = std::uint64_t;

namespace ECS
{
	class WorldChangeJournal;
}

enum class WorldChangeKind : std::uint8_t
{
	WorldReset = 0,
	EntityCreated,
	EntityDestroyed,
	ComponentAdded,
	ComponentRemoved,
	ValueChanged,
	ResourceChanged,
};

enum class WorldDataKind : std::uint8_t
{
	World = 0,
	LocalTransform,
	WorldTransform,
	Camera,
	CameraDerivedState,
	Visibility,
	MeshInstance,
	Light,
	AnimationState,
	MorphState,
	SkinningState,
	SkyEnvironment,
	Material,
	Texture,
	Skeleton,
};

struct WorldChange final
{
	WorldSequence Sequence = 0;
	EntityId Entity;
	WorldChangeKind Kind = WorldChangeKind::ValueChanged;
	WorldDataKind Data = WorldDataKind::World;
};

enum class WorldChangeReadStatus : std::uint8_t
{
	UpToDate = 0,
	Available,
	ResyncRequired,
};

class SPARKLE_ENGINE_API WorldChangeCursor final
{
  public:
	WorldSequence GetAcknowledgedSequence() const noexcept { return m_acknowledgedSequence; }

  private:
	friend class GameWorld;
	WorldSequence m_acknowledgedSequence = 0;
};

class SPARKLE_ENGINE_API WorldChangeBatch final
{
  public:
	WorldChangeBatch() noexcept = default;
	WorldChangeReadStatus GetStatus() const noexcept { return m_status; }
	WorldSequence GetOldestAvailableSequence() const noexcept { return m_oldestAvailableSequence; }
	WorldSequence GetLatestSequence() const noexcept { return m_latestSequence; }
	std::span<const WorldChange> GetChanges() const noexcept;

  private:
	struct Storage;
	friend class ECS::WorldChangeJournal;
	WorldChangeBatch(
	    WorldChangeReadStatus status,
	    WorldSequence oldestAvailableSequence,
	    WorldSequence latestSequence,
	    std::shared_ptr<const Storage> storage,
	    std::size_t firstChangeIndex = 0) noexcept;

	WorldChangeReadStatus m_status = WorldChangeReadStatus::UpToDate;
	WorldSequence m_oldestAvailableSequence = 0;
	WorldSequence m_latestSequence = 0;
	std::shared_ptr<const Storage> m_storage;
	std::size_t m_firstChangeIndex = 0;
};
