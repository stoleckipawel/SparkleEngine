#pragma once

#include "World/ECS/EntityCommandRecord.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace ECS
{
	class EntityCommandCommit;

	class EntityCommandBuffer final
	{
	  public:
		explicit EntityCommandBuffer(EntityCommandBufferDesc desc);

		EntityCommandBuffer(const EntityCommandBuffer&) = delete;
		EntityCommandBuffer& operator=(const EntityCommandBuffer&) = delete;
		EntityCommandBuffer(EntityCommandBuffer&&) noexcept = default;
		EntityCommandBuffer& operator=(EntityCommandBuffer&&) noexcept = default;

		TemporaryEntityId CreateTemporary();
		bool Destroy(EntityId entity);
		bool Destroy(TemporaryEntityId temporary);

		template <ComponentStorageCompatible T> bool Add(EntityId entity, T component)
		{
			return RecordComponent(
			    EntityCommandKind::Add,
			    MakeTarget(entity),
			    std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>(std::move(component)));
		}

		template <ComponentStorageCompatible T> bool Add(TemporaryEntityId temporary, T component)
		{
			return Owns(temporary) && RecordComponent(
			                              EntityCommandKind::Add,
			                              MakeTarget(temporary),
			                              std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>(std::move(component)));
		}

		template <ComponentStorageCompatible T> bool Remove(EntityId entity)
		{
			return RecordComponent(
			    EntityCommandKind::Remove,
			    MakeTarget(entity),
			    std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>());
		}

		template <ComponentStorageCompatible T> bool Remove(TemporaryEntityId temporary)
		{
			return Owns(temporary) && RecordComponent(
			                              EntityCommandKind::Remove,
			                              MakeTarget(temporary),
			                              std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>());
		}

		template <ComponentStorageCompatible T> bool Replace(EntityId entity, T component)
		{
			return RecordComponent(
			    EntityCommandKind::Replace,
			    MakeTarget(entity),
			    std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>(std::move(component)));
		}

		template <ComponentStorageCompatible T> bool Replace(TemporaryEntityId temporary, T component)
		{
			return Owns(temporary) && RecordComponent(
			                              EntityCommandKind::Replace,
			                              MakeTarget(temporary),
			                              std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>(std::move(component)));
		}

		template <ComponentStorageCompatible T> bool Set(EntityId entity, T component)
		{
			return RecordComponent(
			    EntityCommandKind::Set,
			    MakeTarget(entity),
			    std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>(std::move(component)));
		}

		template <ComponentStorageCompatible T> bool Set(TemporaryEntityId temporary, T component)
		{
			return Owns(temporary) && RecordComponent(
			                              EntityCommandKind::Set,
			                              MakeTarget(temporary),
			                              std::make_unique<EntityCommandDetail::TypedComponentCommandOperation<T>>(std::move(component)));
		}

		EntityCommandBufferId GetId() const noexcept { return m_desc.Id; }
		bool HasOverflowed() const noexcept { return m_overflowed; }
		bool HasBeenCommitted() const noexcept { return m_committed; }
		std::size_t GetCommandCount() const noexcept { return m_commands.size(); }

	  private:
		friend class EntityCommandCommit;

		EntityCommandKey NextKey() noexcept;
		bool CanRecord() noexcept;
		bool Owns(TemporaryEntityId temporary) const noexcept;
		bool Record(EntityCommandKind kind, EntityCommandDetail::EntityCommandTarget target);
		bool RecordComponent(
		    EntityCommandKind kind,
		    EntityCommandDetail::EntityCommandTarget target,
		    std::unique_ptr<EntityCommandDetail::ComponentCommandOperation> operation);

		static EntityCommandDetail::EntityCommandTarget MakeTarget(EntityId entity) noexcept;
		static EntityCommandDetail::EntityCommandTarget MakeTarget(TemporaryEntityId temporary) noexcept;

		EntityCommandBufferDesc m_desc;
		std::vector<EntityCommandDetail::EntityCommandRecord> m_commands;
		std::uint32_t m_nextSequence = 0;
		std::uint32_t m_nextTemporaryIndex = 0;
		bool m_overflowed = false;
		bool m_committed = false;
	};
}
