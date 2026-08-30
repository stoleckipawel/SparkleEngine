#include "PCH.h"
#include "World/ECS/EntityCommandCommit.h"

#include "World/ECS/EntityCommandConflictTracker.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace ECS::EntityCommandDetail
{
	EntityCommandBufferId GetBufferId(EntityCommandKey key) noexcept
	{
		return {.System = key.System, .Phase = key.Phase, .Partition = key.Partition};
	}

	EntityId ResolveTemporary(TemporaryEntityId temporary, const std::vector<TemporaryEntityMapping>& mappings) noexcept
	{
		const auto mapping = std::find_if(
		    mappings.begin(),
		    mappings.end(),
		    [&](const TemporaryEntityMapping& candidate) { return candidate.Temporary == temporary; });
		return mapping == mappings.end() ? EntityId::Invalid() : mapping->Entity;
	}

	bool HasDuplicateBufferId(std::span<EntityCommandBuffer* const> buffers)
	{
		std::vector<EntityCommandBufferId> ids;
		ids.reserve(buffers.size());
		for (const EntityCommandBuffer* buffer : buffers)
		{
			if (buffer != nullptr)
			{
				ids.push_back(buffer->GetId());
			}
		}
		std::sort(ids.begin(), ids.end());
		return std::adjacent_find(ids.begin(), ids.end()) != ids.end();
	}
}

namespace ECS
{
	EntityCommandCommitResult EntityCommandCommit::Apply(
	    EntityRegistry& registry,
	    std::span<EntityCommandBuffer* const> buffers,
	    EntityCommandConflictPolicy conflictPolicy)
	{
		EntityCommandCommitResult result;
		if (registry.IsStructureFrozen())
		{
			result.Status = EntityCommandCommitStatus::StructureFrozen;
			return result;
		}
		if (conflictPolicy != EntityCommandConflictPolicy::RejectLaterDeterministicKey)
		{
			result.Status = EntityCommandCommitStatus::InvalidConflictPolicy;
			return result;
		}
		if (EntityCommandDetail::HasDuplicateBufferId(buffers))
		{
			result.Status = EntityCommandCommitStatus::DuplicateBufferId;
			return result;
		}
		for (const EntityCommandBuffer* buffer : buffers)
		{
			if (buffer != nullptr && buffer->HasBeenCommitted())
			{
				result.Status = EntityCommandCommitStatus::BufferAlreadyCommitted;
				return result;
			}
			if (buffer != nullptr && buffer->HasOverflowed())
			{
				result.Status = EntityCommandCommitStatus::BufferOverflow;
				return result;
			}
		}
		for (EntityCommandBuffer* buffer : buffers)
		{
			if (buffer != nullptr)
			{
				buffer->m_committed = true;
			}
		}

		std::vector<EntityCommandDetail::EntityCommandRecord*> commands;
		for (EntityCommandBuffer* buffer : buffers)
		{
			if (buffer == nullptr)
			{
				continue;
			}
			commands.reserve(commands.size() + buffer->m_commands.size());
			for (EntityCommandDetail::EntityCommandRecord& command : buffer->m_commands)
			{
				commands.push_back(&command);
			}
		}
		std::sort(commands.begin(), commands.end(), [](const auto* left, const auto* right) { return left->Key < right->Key; });
		result.Commands.reserve(commands.size());

		EntityCommandDetail::EntityCommandConflictTracker conflicts;
		conflicts.Reserve(commands.size());
		result.TemporaryMappings.reserve(
		    static_cast<std::size_t>(std::count_if(
		        commands.begin(),
		        commands.end(),
		        [](const EntityCommandDetail::EntityCommandRecord* command) { return command->Kind == EntityCommandKind::Create; })));
		for (EntityCommandDetail::EntityCommandRecord* command : commands)
		{
			EntityCommandResult commandResult{.Key = command->Key, .Kind = command->Kind};
			if (command->ComponentOperation != nullptr)
			{
				commandResult.ComponentType = command->ComponentOperation->GetComponentType();
				commandResult.HasComponentType = true;
			}

			if (command->Kind == EntityCommandKind::Create)
			{
				const TemporaryEntityId temporary = command->Target.Temporary;
				if (!command->Target.IsTemporary || !temporary.IsValid()
				    || temporary.GetBufferId() != EntityCommandDetail::GetBufferId(command->Key))
				{
					commandResult.Status = EntityCommandStatus::InvalidTemporaryEntity;
				}
				else
				{
					commandResult.Entity = registry.Create();
					commandResult.Status =
					    commandResult.Entity.IsValid() ? EntityCommandStatus::Applied : EntityCommandStatus::CapacityExceeded;
					if (commandResult.Status == EntityCommandStatus::Applied)
					{
						result.TemporaryMappings.push_back({temporary, commandResult.Entity});
					}
				}
			}
			else
			{
				commandResult.Entity = command->Target.IsTemporary
				    ? EntityCommandDetail::ResolveTemporary(command->Target.Temporary, result.TemporaryMappings)
				    : command->Target.Entity;
				if (!commandResult.Entity.IsValid())
				{
					commandResult.Status =
					    command->Target.IsTemporary ? EntityCommandStatus::InvalidTemporaryEntity : EntityCommandStatus::StaleTarget;
				}
				else if (!registry.IsAlive(commandResult.Entity))
				{
					commandResult.Status = EntityCommandStatus::StaleTarget;
				}
				else if (conflictPolicy == EntityCommandConflictPolicy::RejectLaterDeterministicKey && !command->Target.IsTemporary
				    && !conflicts.TryClaim(
				        commandResult.Entity,
				        EntityCommandDetail::GetBufferId(command->Key),
				        command->Kind,
				        commandResult.ComponentType,
				        commandResult.HasComponentType))
				{
					commandResult.Status = EntityCommandStatus::ConflictRejected;
				}
				else if (command->Kind == EntityCommandKind::Destroy)
				{
					commandResult.Status =
					    registry.Destroy(commandResult.Entity) ? EntityCommandStatus::Applied : EntityCommandStatus::StaleTarget;
				}
				else if (command->ComponentOperation != nullptr)
				{
					commandResult.Status = command->ComponentOperation->Apply(registry, commandResult.Entity, command->Kind);
				}
				else
				{
					commandResult.Status = EntityCommandStatus::StaleTarget;
				}
			}

			if (commandResult.Status != EntityCommandStatus::Applied)
			{
				result.Status = EntityCommandCommitStatus::CompletedWithErrors;
			}
			result.Commands.push_back(commandResult);
		}
		return result;
	}
}
