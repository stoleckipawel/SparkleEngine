#include "PCH.h"

#include "World/Extraction/WorldExtractionStorage.h"

#include "World/ECS/Components/RenderingComponents.h"
#include "World/ECS/EntityRegistry.h"

#include <algorithm>

namespace ECS
{
	bool WorldExtractionStorage::Prepare(const EntityRegistry& registry)
	{
		const ComponentStorage<MeshInstance>* meshes = registry.FindStorage<MeshInstance>();
		const std::size_t meshCount = meshes == nullptr ? 0 : meshes->GetEntities().size();
		if (m_structureVersion != registry.GetStructureVersion() || m_meshSlots.size() != meshCount)
		{
			m_meshSlots.clear();
			m_meshSlots.resize(meshCount);
			m_extractedMeshes.reserve(meshCount);
			m_structureVersion = registry.GetStructureVersion();
		}
		for (MeshSlot& slot : m_meshSlots)
			slot.Included = false;
		return true;
	}

	void WorldExtractionStorage::CommitMeshes(std::span<const SceneMeshInstanceGroupData> groups)
	{
		std::sort(m_meshSlots.begin(), m_meshSlots.end(), [](const MeshSlot& lhs, const MeshSlot& rhs) { return lhs.Entity < rhs.Entity; });
		m_extractedMeshes.clear();
		m_meshGroups.assign(groups.begin(), groups.end());
		for (SceneMeshInstanceGroupData& group : m_meshGroups)
		{
			group.firstInstance = kInvalidSceneMeshInstanceIndex;
			group.instanceCount = 0;
		}
		for (const MeshSlot& slot : m_meshSlots)
		{
			if (!slot.Included)
				continue;
			if (slot.InstanceGroupIndex < m_meshGroups.size())
			{
				SceneMeshInstanceGroupData& group = m_meshGroups[slot.InstanceGroupIndex];
				if (group.instanceCount == 0)
					group.firstInstance = static_cast<SceneMeshInstanceIndex>(m_extractedMeshes.size());
				++group.instanceCount;
			}
			m_extractedMeshes.push_back(slot);
		}
	}
}
