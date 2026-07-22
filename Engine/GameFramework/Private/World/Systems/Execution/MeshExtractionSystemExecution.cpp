#include "PCH.h"

#include "MeshExtractionSystemExecution.h"

#include "World/Extraction/WorldExtractionStorage.h"
#include "World/GameWorldState.h"
#include "World/Resources/SkeletonResourceStore.h"

namespace ECS
{
	MeshExtractionSystemExecution::MeshExtractionSystemExecution(
	    GameWorldState& state,
	    const SkeletonResourceStore& skeletons,
	    const StructureFrozenEpoch& epoch) :
	    m_state(state), m_skeletons(skeletons), m_query(state.m_registry, epoch)
	{
	}

	std::uint32_t MeshExtractionSystemExecution::GetMeshCount() const noexcept
	{
		return static_cast<std::uint32_t>(m_query.GetEstimatedEntityCount());
	}

	bool MeshExtractionSystemExecution::Run(std::uint32_t begin, std::uint32_t end)
	{
		std::span<WorldExtractionStorage::MeshSlot> slots = m_state.m_extraction.GetMeshSlots();
		return m_query.ForEachRange(
		                  begin,
		                  end,
		                  [this, slots](std::size_t index, EntityId entity, const MeshInstance& mesh, const Visibility& visibility, const WorldTransform& world)
		                  {
			                  WorldExtractionStorage::MeshSlot& slot = slots[index];
			                  slot.Entity = entity;
			                  const std::shared_ptr<const Mesh> resource = m_state.m_meshResources.ResolveImmutable(mesh.Resource);
			                  slot.Included = resource != nullptr;
			                  if (!slot.Included) return;
			                  slot.Mesh = ImmutableRenderMeshHandle(mesh.MeshAssetId, resource);
			                  slot.WorldMatrix = world.Matrix;
			                  DirectX::XMStoreFloat3x4(
			                      &slot.WorldInverseTranspose,
			                      DirectX::XMLoadFloat4x4(&world.InverseTranspose));
			                  slot.Material = mesh.Material;
			                  const SkeletonResourceHandle skeleton = m_skeletons.Find(mesh.SkeletonAssetId);
			                  slot.Skeleton = skeleton.IsValid() ? RenderSkeletonAssetHandle(mesh.SkeletonAssetId)
			                                                       : RenderSkeletonAssetHandle{};
			                  slot.Kind = mesh.Kind;
			                  slot.MeshAssetIndex = mesh.MeshAssetIndex;
			                  slot.InstanceGroupIndex = mesh.InstanceGroupIndex;
			                  slot.Visible = visibility.Visible;
		                  })
		    .Succeeded();
	}
}
