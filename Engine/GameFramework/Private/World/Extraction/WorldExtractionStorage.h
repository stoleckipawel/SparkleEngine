#pragma once

#include "GameFramework/Public/Rendering/RenderSceneDelta.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstdint>
#include <DirectXMath.h>

#include <span>
#include <vector>

namespace ECS
{
	class EntityRegistry;

	class WorldExtractionStorage final
	{
	public:
		struct MeshSlot final
		{
			EntityId Entity;
			ImmutableRenderMeshHandle Mesh;
			DirectX::XMFLOAT4X4 WorldMatrix{};
			DirectX::XMFLOAT3X4 WorldInverseTranspose{};
			MaterialHandle Material = MaterialHandle::Invalid();
			RenderSkeletonAssetHandle Skeleton;
			SceneMeshKind Kind = SceneMeshKind::Static;
			SceneMeshAssetIndex MeshAssetIndex = kInvalidSceneMeshAssetIndex;
			SceneMeshInstanceGroupIndex InstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
			bool Visible = true;
			bool Included = false;
		};

		bool Prepare(const EntityRegistry& registry);
		std::span<MeshSlot> GetMeshSlots() noexcept { return m_meshSlots; }
		void CommitMeshes(std::span<const SceneMeshInstanceGroupData> groups);
		std::span<const MeshSlot> GetExtractedMeshes() const noexcept { return m_extractedMeshes; }
		std::span<const SceneMeshInstanceGroupData> GetMeshGroups() const noexcept { return m_meshGroups; }

	private:
		std::vector<MeshSlot> m_meshSlots;
		std::vector<MeshSlot> m_extractedMeshes;
		std::vector<SceneMeshInstanceGroupData> m_meshGroups;
		std::uint64_t m_structureVersion = 0;
	};
}
