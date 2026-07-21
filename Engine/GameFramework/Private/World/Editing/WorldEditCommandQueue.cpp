#include "PCH.h"

#include "WorldEditCommandQueue.h"

#include "World/GameWorldState.h"
#include "World/Resources/GameWorldResourceStores.h"

#include <type_traits>
#include <utility>

WorldEditResult WorldEditCommandQueue::Submit(
	WorldEditCommand command,
	std::uint64_t expectedGeneration,
	std::uint64_t currentGeneration,
	const ECS::GameWorldState& state,
	const GameWorldResourceStores& resources)
{
	WorldEditResult result{.RequestId = command.RequestId};
	if (expectedGeneration != currentGeneration)
	{
		result.Status = WorldEditResultStatus::Stale;
		result.Message = "The editor model belongs to an older world generation.";
		return result;
	}
	if (m_pendingEdits.size() >= MaximumPendingEditCount)
	{
		result.Status = WorldEditResultStatus::Rejected;
		result.Message = "The bounded world-edit queue is full.";
		return result;
	}
	if (!IsTargetAvailable(command.Payload, state, resources))
	{
		result.Status = WorldEditResultStatus::Rejected;
		result.Message = "The semantic edit target or value is unavailable.";
		return result;
	}

	m_pendingEdits.push_back({expectedGeneration, std::move(command)});
	result.Status = WorldEditResultStatus::Accepted;
	return result;
}

void WorldEditCommandQueue::Apply(
	std::uint64_t currentGeneration, ECS::GameWorldState& state, GameWorldResourceStores& resources)
{
	for (PendingEdit& pending : m_pendingEdits)
		if (pending.ExpectedGeneration == currentGeneration)
			ApplyPayload(pending.Command.Payload, state, resources);
	Clear();
}

void WorldEditCommandQueue::Clear() noexcept
{
	m_pendingEdits.clear();
}

bool WorldEditCommandQueue::IsTargetAvailable(
	const WorldEditPayload& payload,
	const ECS::GameWorldState& state,
	const GameWorldResourceStores& resources)
{
	return std::visit(
	    [&state, &resources](const auto& command)
	    {
		    using T = std::decay_t<decltype(command)>;
		    if constexpr (std::is_same_v<T, SetActiveCameraCommand> ||
		                  std::is_same_v<T, SetCameraDescriptionCommand> ||
		                  std::is_same_v<T, SetCameraMovementCommand>)
			    return state.IsCamera(command.Entity);
		    else if constexpr (std::is_same_v<T, SetLocalTransformCommand> ||
		                       std::is_same_v<T, SetEntityVisibilityCommand>)
			    return state.IsAlive(command.Entity);
		    else if constexpr (std::is_same_v<T, SetLightDescriptionCommand>)
			    return state.ReadLight(command.Entity).has_value();
		    else if constexpr (std::is_same_v<T, SetMaterialVariantCommand>)
			    return command.Value < resources.MaterialVariants.GetCount();
		    else
			    return true;
	    },
	    payload);
}

void WorldEditCommandQueue::ApplyPayload(
	WorldEditPayload& payload, ECS::GameWorldState& state, GameWorldResourceStores& resources)
{
	std::visit(
	    [&state, &resources](auto& command)
	    {
		    using T = std::decay_t<decltype(command)>;
		    if constexpr (std::is_same_v<T, SetActiveCameraCommand>)
			    (void) state.SetActiveCamera(command.Entity);
		    else if constexpr (std::is_same_v<T, SetLocalTransformCommand>)
			    (void) state.WriteTransform(command.Entity, command.Value);
		    else if constexpr (std::is_same_v<T, SetCameraDescriptionCommand>)
			    (void) state.WriteCameraDesc(command.Entity, command.Value);
		    else if constexpr (std::is_same_v<T, SetCameraMovementCommand>)
			    (void) state.WriteCameraMovement(command.Entity, command.Value);
		    else if constexpr (std::is_same_v<T, SetEntityVisibilityCommand>)
			    (void) state.WriteVisibility(command.Entity, command.Value);
		    else if constexpr (std::is_same_v<T, SetLightDescriptionCommand>)
			    (void) state.WriteLight(command.Entity, std::move(command.Value));
		    else if constexpr (std::is_same_v<T, SetSkyEnvironmentCommand>)
		    {
			    if (command.Value)
				    state.WriteSkyEnvironment(std::move(*command.Value));
			    else
				    state.RemoveSkyEnvironment();
		    }
		    else if constexpr (std::is_same_v<T, SetMaterialVariantCommand>)
			    (void) resources.MaterialVariants.Apply(command.Value, state);
	    },
	    payload);
}
