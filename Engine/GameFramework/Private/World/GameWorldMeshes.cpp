#include "PCH.h"
#include "World/GameWorldState.h"

#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"
#include "World/ECS/Components/EditorComponents.h"
#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/Components/TransformComponents.h"
#include "World/WorldTransformConversion.h"

namespace ECS
{
	EntityId GameWorldState::AddMesh(SceneMeshInstanceData&& instance)
	{
		const SceneMeshResourceHandle resource = m_meshResources.Add(std::move(instance.Resource));
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
		const SceneStateHandle morphState = instance.Kind == SceneMeshKind::Skeletal
		                                        ? m_deformationStates.AddMorphWeights(instance.InitialMorphWeights)
		                                        : SceneStateHandle{};
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
				m_deformationStates.Remove(morphState);
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

	void GameWorldState::AppendMeshInstanceGroups(std::vector<MeshInstanceGroupSnapshot>&& groups)
	{
		m_meshInstanceGroups.reserve(m_meshInstanceGroups.size() + groups.size());
		for (MeshInstanceGroupSnapshot& group : groups)
		{
			m_meshInstanceGroups.push_back(std::move(group));
		}
		RecordChange(EntityId::Invalid(), WorldChangeKind::ResourceChanged, WorldDataKind::MeshInstance);
	}

	MeshSnapshot GameWorldState::CaptureMeshes() const
	{
		MeshSnapshot snapshot;
		const ComponentStorage<MeshInstance>* meshes = m_registry.FindStorage<MeshInstance>();
		if (meshes == nullptr)
		{
			return snapshot;
		}
		snapshot.meshInstances.reserve(meshes->GetEntities().size());
		snapshot.meshInstanceGroups = m_meshInstanceGroups;
		for (MeshInstanceGroupSnapshot& group : snapshot.meshInstanceGroups)
		{
			group.firstInstance = kInvalidSceneMeshInstanceIndex;
			group.instanceCount = 0;
		}

		const std::span<const EntityId> entities = meshes->GetEntities();
		const std::span<const MeshInstance> components = meshes->GetComponents();
		for (std::size_t index = 0; index < entities.size(); ++index)
		{
			const EntityId entity = entities[index];
			const MeshInstance& mesh = components[index];
			const Mesh* resource = m_meshResources.Resolve(mesh.Resource);
			if (!ReadVisibility(entity) || resource == nullptr)
			{
				continue;
			}
			const WorldTransform* transform = m_registry.Get<WorldTransform>(entity);
			if (transform == nullptr)
			{
				continue;
			}
			MeshInstanceSnapshot instance;
			instance.mesh = resource;
			instance.worldMatrix = transform->Matrix;
			DirectX::XMStoreFloat3x4(&instance.worldInvTranspose, DirectX::XMLoadFloat4x4(&transform->InverseTranspose));
			instance.materialHandle = mesh.Material;
			instance.meshAssetId = mesh.MeshAssetId;
			instance.skeletonAssetId = mesh.SkeletonAssetId;
			instance.meshKind = mesh.Kind;
			instance.meshAssetIndex = mesh.MeshAssetIndex;
			instance.instanceGroupIndex = mesh.InstanceGroupIndex;
			if (instance.instanceGroupIndex < snapshot.meshInstanceGroups.size())
			{
				MeshInstanceGroupSnapshot& group = snapshot.meshInstanceGroups[instance.instanceGroupIndex];
				if (group.instanceCount == 0)
				{
					group.firstInstance = static_cast<SceneMeshInstanceIndex>(snapshot.meshInstances.size());
				}
				++group.instanceCount;
			}
			snapshot.meshInstances.push_back(instance);
		}
		return snapshot;
	}

	void GameWorldState::ApplyMorphWeights(std::span<const SceneMorphWeightSnapshot> weights)
	{
		if (weights.empty())
		{
			return;
		}
		const ComponentStorage<MeshInstance>* meshes = m_registry.FindStorage<MeshInstance>();
		if (meshes == nullptr)
		{
			return;
		}
		for (const SceneMorphWeightSnapshot& weightsForNode : weights)
		{
			const std::span<const EntityId> entities = meshes->GetEntities();
			const std::span<const MeshInstance> components = meshes->GetComponents();
			for (std::size_t index = 0; index < components.size(); ++index)
			{
				const MeshInstance& mesh = components[index];
				if (mesh.Kind != SceneMeshKind::Skeletal || mesh.SourceNodeIndex != weightsForNode.targetNodeIndex)
				{
					continue;
				}
				const MorphState* morph = m_registry.Get<MorphState>(entities[index]);
				if (morph == nullptr || !m_deformationStates.WriteMorphWeights(morph->Weights, weightsForNode.weights))
				{
					continue;
				}
				if (auto* skeletal = dynamic_cast<SkeletalCookedMesh*>(m_meshResources.Resolve(mesh.Resource)))
				{
					skeletal->SetMorphWeights(weightsForNode.weights);
				}
				RecordChange(entities[index], WorldChangeKind::ValueChanged, WorldDataKind::MorphState);
			}
		}
	}
}
