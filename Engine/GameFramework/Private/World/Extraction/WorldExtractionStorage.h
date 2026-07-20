#pragma once

#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"
#include "GameFramework/Public/World/EntityId.h"

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
			MeshInstanceSnapshot Snapshot;
			bool Included = false;
		};

		bool Prepare(const EntityRegistry& registry);
		std::span<MeshSlot> GetMeshSlots() noexcept { return m_meshSlots; }
		void CommitMeshes(std::span<const MeshInstanceGroupSnapshot> groups);
		const MeshSnapshot& GetMeshes() const noexcept { return m_meshes; }

	  private:
		std::vector<MeshSlot> m_meshSlots;
		MeshSnapshot m_meshes;
		std::uint64_t m_structureVersion = 0;
	};
}
