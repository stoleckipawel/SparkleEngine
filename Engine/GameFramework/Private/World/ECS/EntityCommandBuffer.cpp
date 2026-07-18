#include "PCH.h"
#include "World/ECS/EntityCommandBuffer.h"

#include <limits>
#include <utility>

namespace ECS
{
	EntityCommandBuffer::EntityCommandBuffer(EntityCommandBufferDesc desc) : m_desc(desc)
	{
		m_commands.reserve(m_desc.MaxCommands);
	}

	TemporaryEntityId EntityCommandBuffer::CreateTemporary()
	{
		if (m_nextTemporaryIndex >= m_desc.MaxTemporaryEntities || m_nextTemporaryIndex == TemporaryEntityId::InvalidLocalIndex)
		{
			m_overflowed = true;
			return {};
		}

		const TemporaryEntityId temporary(m_desc.Id, m_nextTemporaryIndex++);
		if (!Record(EntityCommandKind::Create, MakeTarget(temporary)))
		{
			return {};
		}
		return temporary;
	}

	bool EntityCommandBuffer::Destroy(EntityId entity)
	{
		return Record(EntityCommandKind::Destroy, MakeTarget(entity));
	}

	bool EntityCommandBuffer::Destroy(TemporaryEntityId temporary)
	{
		return Owns(temporary) && Record(EntityCommandKind::Destroy, MakeTarget(temporary));
	}

	EntityCommandKey EntityCommandBuffer::NextKey() noexcept
	{
		return EntityCommandKey{
		    .System = m_desc.Id.System,
		    .Phase = m_desc.Id.Phase,
		    .Partition = m_desc.Id.Partition,
		    .LocalSequence = m_nextSequence++};
	}

	bool EntityCommandBuffer::CanRecord() noexcept
	{
		if (m_committed || m_overflowed || m_commands.size() >= m_desc.MaxCommands ||
		    m_nextSequence == (std::numeric_limits<std::uint32_t>::max)())
		{
			m_overflowed = true;
			return false;
		}
		return true;
	}

	bool EntityCommandBuffer::Owns(TemporaryEntityId temporary) const noexcept
	{
		return temporary.IsValid() && temporary.GetBufferId() == m_desc.Id && temporary.GetLocalIndex() < m_nextTemporaryIndex;
	}

	bool EntityCommandBuffer::Record(EntityCommandKind kind, EntityCommandDetail::EntityCommandTarget target)
	{
		if (!CanRecord())
		{
			return false;
		}
		m_commands.push_back(EntityCommandDetail::EntityCommandRecord{.Key = NextKey(), .Kind = kind, .Target = target});
		return true;
	}

	bool EntityCommandBuffer::RecordComponent(
	    EntityCommandKind kind,
	    EntityCommandDetail::EntityCommandTarget target,
	    std::unique_ptr<EntityCommandDetail::ComponentCommandOperation> operation)
	{
		if (!CanRecord())
		{
			return false;
		}
		m_commands.push_back(
		    EntityCommandDetail::EntityCommandRecord{
		        .Key = NextKey(),
		        .Kind = kind,
		        .Target = target,
		        .ComponentOperation = std::move(operation)});
		return true;
	}

	EntityCommandDetail::EntityCommandTarget EntityCommandBuffer::MakeTarget(EntityId entity) noexcept
	{
		return {.Entity = entity};
	}

	EntityCommandDetail::EntityCommandTarget EntityCommandBuffer::MakeTarget(TemporaryEntityId temporary) noexcept
	{
		return {.Temporary = temporary, .IsTemporary = true};
	}
}
