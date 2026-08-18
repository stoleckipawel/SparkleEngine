#include "PCH.h"
#include "World/GameWorldState.h"

#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/Publication/WorldReadViewStorage.h"

#include <algorithm>

class WorldReadViewEntityIndex final
{
  public:
	template <typename T> static void EraseEntity(std::vector<T>& values, EntityId entity)
	{
		std::erase_if(values, [entity](const T& value) { return value.Entity == entity; });
	}

	template <typename T> static void UpsertEntity(std::vector<T>& values, T value)
	{
		auto position = std::lower_bound(
		    values.begin(), values.end(), value.Entity, [](const T& existing, EntityId entity) { return existing.Entity < entity; });
		if (position != values.end() && position->Entity == value.Entity)
		{
			*position = std::move(value);
		}
		else
		{
			values.insert(position, std::move(value));
		}
	}
};

namespace ECS
{
	WorldCameraReadData GameWorldState::BuildCameraReadData(EntityId entity) const
	{
		WorldCameraReadData read;
		read.Entity = entity;
		if (const std::optional<SceneCameraEntry> entry = ReadCamera(entity))
		{
			read.Name = entry->name;
			read.Description = entry->desc;
		}
		read.LocalTransform = ReadTransform(entity);
		if (const WorldTransform* world = m_registry.Get<WorldTransform>(entity))
		{
			read.WorldMatrix = world->Matrix;
		}
		if (const CameraDerivedState* derived = m_registry.Get<CameraDerivedState>(entity))
		{
			read.Direction = derived->Direction;
		}
		if (const Camera* camera = m_registry.Get<Camera>(entity))
		{
			read.AspectRatio = camera->AspectRatio;
			read.Active = camera->Active;
		}
		read.Visible = ReadVisibility(entity);
		return read;
	}

	WorldLightReadData GameWorldState::BuildLightReadData(EntityId entity) const
	{
		WorldLightReadData read;
		read.Entity = entity;
		if (std::optional<SceneLightDesc> light = ReadLight(entity))
		{
			read.Description = std::move(*light);
		}
		return read;
	}

	WorldMeshReadData GameWorldState::BuildMeshReadData(EntityId entity) const
	{
		WorldMeshReadData read;
		read.Entity = entity;
		read.LocalTransform = ReadTransform(entity);
		if (const WorldTransform* world = m_registry.Get<WorldTransform>(entity))
		{
			read.WorldMatrix = world->Matrix;
		}
		if (const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity))
		{
			read.Material = mesh->Material;
			read.MeshAssetId = mesh->MeshAssetId;
			read.SkeletonAssetId = mesh->SkeletonAssetId;
			read.Kind = mesh->Kind;
			read.SourceNodeIndex = mesh->SourceNodeIndex;
		}
		read.Visible = ReadVisibility(entity);
		return read;
	}

	void GameWorldState::PublishReadView(
	    std::span<const WorldChange> changes,
	    WorldSequence sequence,
	    bool fullBaseline)
	{
		const std::shared_ptr<const WorldReadView::Storage> previous = m_publishedReadView.load(std::memory_order_acquire);
		auto next = previous != nullptr && !fullBaseline
		                ? std::make_shared<WorldReadView::Storage>(*previous)
		                : std::make_shared<WorldReadView::Storage>();
		next->Generation = ++m_readGeneration;
		next->Sequence = sequence;

		if (previous == nullptr || fullBaseline)
		{
			next->Cameras.clear();
			next->Lights.clear();
			next->Meshes.clear();
			if (const ComponentStorage<Camera>* cameras = m_registry.FindStorage<Camera>())
			{
				for (EntityId entity : cameras->GetEntities())
				{
					next->Cameras.push_back(BuildCameraReadData(entity));
				}
			}
			if (const ComponentStorage<Light>* lights = m_registry.FindStorage<Light>())
			{
				for (EntityId entity : lights->GetEntities())
				{
					next->Lights.push_back(BuildLightReadData(entity));
				}
			}
			if (const ComponentStorage<MeshInstance>* meshes = m_registry.FindStorage<MeshInstance>())
			{
				for (EntityId entity : meshes->GetEntities())
				{
					next->Meshes.push_back(BuildMeshReadData(entity));
				}
			}
			std::sort(next->Cameras.begin(), next->Cameras.end(), [](const auto& a, const auto& b) { return a.Entity < b.Entity; });
			std::sort(next->Lights.begin(), next->Lights.end(), [](const auto& a, const auto& b) { return a.Entity < b.Entity; });
			std::sort(next->Meshes.begin(), next->Meshes.end(), [](const auto& a, const auto& b) { return a.Entity < b.Entity; });
		}
		else
		{
			std::vector<EntityId> changedEntities;
			for (const WorldChange& change : changes)
			{
				if (change.Entity.IsValid())
				{
					changedEntities.push_back(change.Entity);
				}
			}
			std::sort(changedEntities.begin(), changedEntities.end());
			changedEntities.erase(std::unique(changedEntities.begin(), changedEntities.end()), changedEntities.end());
			for (EntityId entity : changedEntities)
			{
				WorldReadViewEntityIndex::EraseEntity(next->Cameras, entity);
				WorldReadViewEntityIndex::EraseEntity(next->Lights, entity);
				WorldReadViewEntityIndex::EraseEntity(next->Meshes, entity);
				if (m_registry.Get<Camera>(entity) != nullptr)
				{
					WorldReadViewEntityIndex::UpsertEntity(next->Cameras, BuildCameraReadData(entity));
				}
				if (m_registry.Get<Light>(entity) != nullptr)
				{
					WorldReadViewEntityIndex::UpsertEntity(next->Lights, BuildLightReadData(entity));
				}
				if (m_registry.Get<MeshInstance>(entity) != nullptr)
				{
					WorldReadViewEntityIndex::UpsertEntity(next->Meshes, BuildMeshReadData(entity));
				}
			}
		}
		next->Sky = m_skyEnvironment;
		m_publishedReadView.store(std::move(next), std::memory_order_release);
	}
}
