#pragma once

#include "GameFramework/Public/World/EntityId.h"
#include "World/ECS/ComponentTypeRegistry.h"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace ECS
{
	struct EntityCommandBufferId final
	{
		std::uint32_t System = 0;
		std::uint32_t Phase = 0;
		std::uint32_t Partition = 0;

		constexpr auto operator<=>(const EntityCommandBufferId&) const noexcept = default;
	};

	struct EntityCommandKey final
	{
		std::uint32_t System = 0;
		std::uint32_t Phase = 0;
		std::uint32_t Partition = 0;
		std::uint32_t LocalSequence = 0;

		constexpr auto operator<=>(const EntityCommandKey&) const noexcept = default;
	};

	class TemporaryEntityId final
	{
	public:
		static constexpr std::uint32_t InvalidLocalIndex = (std::numeric_limits<std::uint32_t>::max)();

		constexpr TemporaryEntityId() noexcept = default;
		constexpr bool IsValid() const noexcept { return m_localIndex != InvalidLocalIndex; }
		constexpr EntityCommandBufferId GetBufferId() const noexcept { return m_bufferId; }
		constexpr std::uint32_t GetLocalIndex() const noexcept { return m_localIndex; }
		constexpr auto operator<=>(const TemporaryEntityId&) const noexcept = default;

	private:
		friend class EntityCommandBuffer;

		constexpr TemporaryEntityId(EntityCommandBufferId bufferId, std::uint32_t localIndex) noexcept :
		    m_bufferId(bufferId),
		    m_localIndex(localIndex)
		{
		}

		EntityCommandBufferId m_bufferId;
		std::uint32_t m_localIndex = InvalidLocalIndex;
	};

	struct EntityCommandBufferDesc final
	{
		EntityCommandBufferId Id;
		std::size_t MaxCommands = 1024;
		std::size_t MaxTemporaryEntities = 256;
	};

	enum class EntityCommandKind : std::uint8_t
	{
		Create,
		Destroy,
		Add,
		Remove,
		Replace,
		Set,
	};

	enum class EntityCommandStatus : std::uint8_t
	{
		Applied,
		StaleTarget,
		ComponentMissing,
		ComponentAlreadyPresent,
		InvalidTemporaryEntity,
		ConflictRejected,
		CapacityExceeded,
	};

	enum class EntityCommandCommitStatus : std::uint8_t
	{
		Completed,
		CompletedWithErrors,
		StructureFrozen,
		DuplicateBufferId,
		BufferOverflow,
		BufferAlreadyCommitted,
		InvalidConflictPolicy,
	};

	enum class EntityCommandConflictPolicy : std::uint8_t
	{
		RejectLaterDeterministicKey,
	};

	struct EntityCommandResult final
	{
		EntityCommandKey Key;
		EntityCommandKind Kind = EntityCommandKind::Create;
		EntityCommandStatus Status = EntityCommandStatus::StaleTarget;
		EntityId Entity;
		RuntimeComponentTypeId ComponentType;
		bool HasComponentType = false;
	};

	struct TemporaryEntityMapping final
	{
		TemporaryEntityId Temporary;
		EntityId Entity;
	};

	struct EntityCommandCommitResult final
	{
		EntityCommandCommitStatus Status = EntityCommandCommitStatus::Completed;
		std::vector<EntityCommandResult> Commands;
		std::vector<TemporaryEntityMapping> TemporaryMappings;

		bool Completed() const noexcept
		{
			return Status == EntityCommandCommitStatus::Completed || Status == EntityCommandCommitStatus::CompletedWithErrors;
		}
		bool AllApplied() const noexcept { return Status == EntityCommandCommitStatus::Completed; }
	};
}
