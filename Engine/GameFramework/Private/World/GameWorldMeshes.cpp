#include "PCH.h"
#include "World/GameWorldState.h"

#include "Scene/Meshes/Mesh.h"
#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/WorldTransformConversion.h"

namespace ECS
{
	EntityId GameWorldState::AddMesh(SceneMeshInstanceData&& instance)
	{
		const MeshResourceHandle resource = m_meshResources.Add(std::move(instance.Resource));
		if (!resource.IsValid())
		{
			return EntityId::Invalid();
		}
		const EntityId entity = m_registry.Create();
		if (!entity.IsValid())
		{
			m_meshResources.Remove(resource);
			return entity;
		}
		const LocalTransform local = WorldTransformConversion::ToLocal(instance.LocalTransform);
		const AnimationOutputSlotHandle morphState = instance.Kind == SceneMeshKind::Skeletal
		                                                ? m_morphWeights.Add(instance.InitialMorphWeights)
		                                                : AnimationOutputSlotHandle{};
		const MeshInstance mesh{
		    .Resource = resource,
		    .MeshAssetId = instance.MeshAssetId,
		    .SkeletonAssetId = instance.SkeletonAssetId,
		    .Material = instance.Material,
		    .Kind = instance.Kind,
		    .MeshAssetIndex = instance.MeshAssetIndex,
		    .InstanceGroupIndex = instance.InstanceGroupIndex,
		    .SourceNodeIndex = instance.SourceNodeIndex};
		bool added = m_registry.Add(entity, local) &&
		                   m_registry.Add(entity, WorldTransform{}) &&
		                   m_registry.Add(entity, mesh) &&
		                   m_registry.Add(entity, Visibility{}) &&
		                   m_registry.Add(
		                       entity,
		                       AuthoredIdentity{
		                           .SourceAssetId = instance.MeshAssetId,
		                           .SourceInstanceId = instance.SourceInstanceId,
		                           .SourceObjectId = static_cast<std::uint64_t>(instance.SourceNodeIndex),
		                           .Kind = AuthoredObjectKind::MeshInstance}) &&
		                   m_registry.Add(entity, EditorMetadata{});
		if (added && instance.Kind == SceneMeshKind::Skeletal)
		{
			added = morphState.IsValid() &&
			        m_registry.Add(entity, MorphState{.Weights = morphState}) &&
			        m_registry.Add(entity, SkinningState{.SkeletonAssetId = instance.SkeletonAssetId});
		}
		if (!added)
		{
			m_registry.Destroy(entity);
			m_meshResources.Remove(resource);
			if (morphState.IsValid())
			{
				m_morphWeights.Remove(morphState);
			}
			return EntityId::Invalid();
		}
		MarkTransformDirty(entity);
		RecordChange(entity, WorldChangeKind::EntityCreated, WorldDataKind::World);
		RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::MeshInstance);
		RecordChange(entity, WorldChangeKind::ComponentAdded, WorldDataKind::LocalTransform);
		return entity;
	}

	std::size_t GameWorldState::GetMeshCount() const noexcept { return Count<MeshInstance>(); }
	EntityId GameWorldState::GetMeshEntity(std::size_t index) const noexcept { return EntityAt<MeshInstance>(index); }

	const Mesh* GameWorldState::ResolveMesh(EntityId entity) const noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		return mesh == nullptr ? nullptr : m_meshResources.Resolve(mesh->Resource);
	}

	bool GameWorldState::IsSkeletalMesh(EntityId entity) const noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		return mesh != nullptr && mesh->Kind == SceneMeshKind::Skeletal;
	}

	MaterialHandle GameWorldState::ReadMeshMaterial(EntityId entity) const noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		return mesh == nullptr ? MaterialHandle::Invalid() : mesh->Material;
	}

	bool GameWorldState::WriteMeshMaterial(EntityId entity, MaterialHandle material) noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		if (mesh == nullptr)
		{
			return false;
		}
		MeshInstance updated = *mesh;
		updated.Material = material;
		const bool written = m_registry.Replace(entity, updated);
		if (written)
		{
			RecordChange(entity, WorldChangeKind::ValueChanged, WorldDataKind::MeshInstance);
		}
		return written;
	}

	Assets::CookedAssetId GameWorldState::ReadMeshAssetId(EntityId entity) const noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		return mesh == nullptr ? Assets::InvalidCookedAssetId : mesh->MeshAssetId;
	}

	Assets::CookedAssetId GameWorldState::ReadSkeletonAssetId(EntityId entity) const noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		return mesh == nullptr ? Assets::InvalidCookedAssetId : mesh->SkeletonAssetId;
	}

	std::uint32_t GameWorldState::ReadMeshSourceNodeIndex(EntityId entity) const noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		return mesh == nullptr ? Assets::kInvalidCookedSceneSourceNodeIndex : mesh->SourceNodeIndex;
	}

	void GameWorldState::AppendMeshInstanceGroups(std::vector<SceneMeshInstanceGroupData>&& groups)
	{
		m_meshInstanceGroups.reserve(m_meshInstanceGroups.size() + groups.size());
		for (SceneMeshInstanceGroupData& group : groups)
		{
			m_meshInstanceGroups.push_back(std::move(group));
		}
		RecordChange(EntityId::Invalid(), WorldChangeKind::ResourceChanged, WorldDataKind::MeshInstance);
	}

}
