#include "PCH.h"

#include "SystemChangeCommitter.h"

#include "World/GameWorldState.h"

#include <algorithm>

namespace ECS
{
	void SystemChangeCommitter::SortUnique(std::vector<EntityId>& entities)
	{
		std::sort(entities.begin(), entities.end());
		entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
	}

	void SystemChangeCommitter::RecordTransformChanges(GameWorldState& state, std::vector<EntityId>& changes)
	{
		std::erase(changes, EntityId::Invalid());
		SortUnique(changes);
		for (EntityId entity : changes)
		{
			state.m_systemArena.DirtyTransforms.push_back(entity);
			state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::LocalTransform);
		}
	}

	bool SystemChangeCommitter::CommitSystemOutputs(GameWorldState& state)
	{
		RecordTransformChanges(state, state.m_systemArena.CameraChanges);
		std::erase(state.m_systemArena.AnimationChanges, EntityId::Invalid());
		SortUnique(state.m_systemArena.AnimationChanges);
		for (EntityId entity : state.m_systemArena.AnimationChanges)
			state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::AnimationState);
		std::erase(state.m_systemArena.MorphChanges, EntityId::Invalid());
		SortUnique(state.m_systemArena.MorphChanges);
		for (EntityId entity : state.m_systemArena.MorphChanges)
			state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::MorphState);
		SortUnique(state.m_systemArena.DirtyTransforms);
		return true;
	}

	bool SystemChangeCommitter::CommitExtraction(GameWorldState& state)
	{
		state.m_extraction.CommitMeshes(state.m_meshInstanceGroups);
		std::erase(state.m_systemArena.EvaluatedTransforms, EntityId::Invalid());
		SortUnique(state.m_systemArena.EvaluatedTransforms);
		for (EntityId entity : state.m_systemArena.EvaluatedTransforms)
			state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::WorldTransform);
		std::erase(state.m_systemArena.CameraDerivedChanges, EntityId::Invalid());
		SortUnique(state.m_systemArena.CameraDerivedChanges);
		for (EntityId entity : state.m_systemArena.CameraDerivedChanges)
			state.RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::CameraDerivedState);
		state.PublishPendingChanges();
		return true;
	}
}
