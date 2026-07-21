#pragma once

#include "World/ECS/Components/RenderingComponents.h"
#include "GameFramework/Public/Scene/Meshes/Mesh.h"

#include <memory>
#include <vector>

namespace ECS
{
	class MeshResourceStore final
	{
	  public:
		MeshResourceHandle Add(std::unique_ptr<Mesh>&& mesh);
		Mesh* Resolve(MeshResourceHandle handle) noexcept;
		const Mesh* Resolve(MeshResourceHandle handle) const noexcept;
		std::shared_ptr<const Mesh> ResolveImmutable(MeshResourceHandle handle) const noexcept;
		bool Remove(MeshResourceHandle handle) noexcept;
		void Clear() noexcept;

	  private:
		struct Entry final
		{
			std::shared_ptr<Mesh> Resource;
			std::uint32_t Generation = 1;
		};

		std::vector<Entry> m_entries;
		std::vector<std::uint32_t> m_freeSlots;
	};
}
