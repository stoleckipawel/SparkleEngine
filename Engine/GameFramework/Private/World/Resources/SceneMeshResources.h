#pragma once

#include "World/ECS/Components/RenderingComponents.h"
#include "GameFramework/Public/Scene/Meshes/Mesh.h"

#include <memory>
#include <vector>

namespace ECS
{
	class SceneMeshResources final
	{
	  public:
		SceneMeshResourceHandle Add(std::unique_ptr<Mesh>&& mesh);
		Mesh* Resolve(SceneMeshResourceHandle handle) noexcept;
		const Mesh* Resolve(SceneMeshResourceHandle handle) const noexcept;
		bool Remove(SceneMeshResourceHandle handle) noexcept;
		void Clear() noexcept;

	  private:
		struct Entry final
		{
			std::unique_ptr<Mesh> Resource;
			std::uint32_t Generation = 1;
		};

		std::vector<Entry> m_entries;
		std::vector<std::uint32_t> m_freeSlots;
	};
}
