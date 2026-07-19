#include "PCH.h"
#include "World/GameWorldState.h"

#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"

#include "World/Systems/CameraDerivedStateEvaluationSystem.h"
#include "World/Systems/TransformEvaluationSystem.h"

#include <algorithm>

namespace ECS
{
	GameWorldState::GameWorldState()
	{
		m_dirtyTransforms.reserve(WorldChangeJournal::MaxChangesPerBatch);
		m_pendingChanges.reserve(WorldChangeJournal::MaxChangesPerBatch);
		CommitDerivedStateAndPublish();
	}

	bool GameWorldState::Destroy(EntityId entity) noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		const SceneMeshResourceHandle meshResource = mesh == nullptr ? SceneMeshResourceHandle{} : mesh->Resource;
		const MorphState* morph = m_registry.Get<MorphState>(entity);
		const SceneStateHandle morphState = morph == nullptr ? SceneStateHandle{} : morph->Weights;
		const bool destroyed = m_registry.Destroy(entity);
		if (!destroyed)
		{
			return false;
		}
		if (entity == m_activeCamera)
		{
			m_activeCamera = EntityId::Invalid();
		}
		if (meshResource.IsValid())
		{
			m_meshResources.Remove(meshResource);
		}
		if (morphState.IsValid())
		{
			m_deformationStates.Remove(morphState);
		}
		RecordChange(entity, WorldChangeKind::EntityDestroyed, WorldDataKind::World);
		return true;
	}

	void GameWorldState::MarkTransformDirty(EntityId entity) noexcept
	{
		if (!entity.IsValid())
		{
			return;
		}
		if (m_dirtyTransforms.size() < m_dirtyTransforms.capacity())
		{
			m_dirtyTransforms.push_back(entity);
		}
		else
		{
			m_evaluateAllTransforms = true;
		}
	}

	void GameWorldState::RecordChange(EntityId entity, WorldChangeKind kind, WorldDataKind data) noexcept
	{
		if (m_changesOverflowed)
		{
			return;
		}
		if (m_pendingChanges.size() < m_pendingChanges.capacity())
		{
			m_pendingChanges.push_back(WorldChange{.Entity = entity, .Kind = kind, .Data = data});
			return;
		}
		m_pendingChanges.clear();
		m_pendingChanges.push_back(WorldChange{.Kind = WorldChangeKind::WorldReset, .Data = WorldDataKind::World});
		m_forceFullPublication = true;
		m_changesOverflowed = true;
	}

	void GameWorldState::WriteSkyEnvironment(SkyEnvironment environment) noexcept
	{
		m_skyEnvironment = std::move(environment);
		RecordChange(EntityId::Invalid(), WorldChangeKind::ResourceChanged, WorldDataKind::SkyEnvironment);
	}

	void GameWorldState::RemoveSkyEnvironment() noexcept
	{
		if (m_skyEnvironment.has_value())
		{
			m_skyEnvironment.reset();
			RecordChange(EntityId::Invalid(), WorldChangeKind::ResourceChanged, WorldDataKind::SkyEnvironment);
		}
	}

	void GameWorldState::NotifyResourceChanged(WorldDataKind data) noexcept
	{
		RecordChange(EntityId::Invalid(), WorldChangeKind::ResourceChanged, data);
	}

	void GameWorldState::CommitDerivedStateAndPublish()
	{
		std::span<const EntityId> dirtyEntities;
		if (m_evaluateAllTransforms)
		{
			if (const ComponentStorage<LocalTransform>* transforms = m_registry.FindStorage<LocalTransform>())
			{
				dirtyEntities = transforms->GetEntities();
			}
		}
		else
		{
			std::sort(m_dirtyTransforms.begin(), m_dirtyTransforms.end());
			m_dirtyTransforms.erase(std::unique(m_dirtyTransforms.begin(), m_dirtyTransforms.end()), m_dirtyTransforms.end());
			dirtyEntities = m_dirtyTransforms;
		}
		if (!dirtyEntities.empty())
		{
			TransformEvaluationSystem::Evaluate(m_registry, dirtyEntities);
			CameraDerivedStateEvaluationSystem::Evaluate(m_registry, dirtyEntities);
			for (EntityId entity : dirtyEntities)
			{
				if (!m_registry.IsAlive(entity))
				{
					continue;
				}
				RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::WorldTransform);
				if (m_registry.Get<Camera>(entity) != nullptr)
				{
					RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::CameraDerivedState);
				}
			}
		}
		m_dirtyTransforms.clear();
		m_evaluateAllTransforms = false;

		if (m_pendingChanges.empty() && m_publishedReadView.load(std::memory_order_acquire) != nullptr)
		{
			return;
		}
		const bool fullBaseline = m_forceFullPublication || m_pendingChanges.size() > WorldChangeJournal::MaxChangesPerBatch;
		const WorldSequence previousSequence = m_changeJournal.GetLatestSequence();
		const WorldSequence sequence = m_changeJournal.Publish(m_pendingChanges);
		m_pendingChanges.clear();
		m_changesOverflowed = false;
		const WorldChangeBatch batch = m_changeJournal.ReadAfter(previousSequence);
		PublishReadView(batch.GetChanges(), sequence, fullBaseline);
		m_forceFullPublication = false;
	}

	WorldReadView GameWorldState::AcquireReadView() const noexcept
	{
		return WorldReadView(m_publishedReadView.load(std::memory_order_acquire));
	}

	WorldChangeBatch GameWorldState::ReadChanges(WorldSequence acknowledgedSequence) const
	{
		return m_changeJournal.ReadAfter(acknowledgedSequence);
	}
}
