#include "PCH.h"
#include "World/Resources/SceneMeshResources.h"

#include "Scene/Meshes/Mesh.h"

#include <limits>

namespace ECS
{
	SceneMeshResourceHandle SceneMeshResources::Add(std::unique_ptr<Mesh>&& mesh)
	{
		if (!mesh)
		{
			return {};
		}
		if (!m_freeSlots.empty())
		{
			const std::uint32_t slot = m_freeSlots.back();
			m_freeSlots.pop_back();
			Entry& entry = m_entries[slot];
			entry.Resource = std::move(mesh);
			return SceneMeshResourceHandle{slot, entry.Generation};
		}
		if (m_entries.size() >= SceneMeshResourceHandle{}.Slot)
		{
			return {};
		}

		const auto slot = static_cast<std::uint32_t>(m_entries.size());
		m_entries.push_back(Entry{.Resource = std::move(mesh)});
		return SceneMeshResourceHandle{slot, m_entries.back().Generation};
	}

	Mesh* SceneMeshResources::Resolve(SceneMeshResourceHandle handle) noexcept
	{
		return const_cast<Mesh*>(std::as_const(*this).Resolve(handle));
	}

	const Mesh* SceneMeshResources::Resolve(SceneMeshResourceHandle handle) const noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
		{
			return nullptr;
		}
		const Entry& entry = m_entries[handle.Slot];
		return entry.Generation == handle.Generation ? entry.Resource.get() : nullptr;
	}

	bool SceneMeshResources::Remove(SceneMeshResourceHandle handle) noexcept
	{
		if (!handle.IsValid() || handle.Slot >= m_entries.size())
		{
			return false;
		}
		Entry& entry = m_entries[handle.Slot];
		if (entry.Generation != handle.Generation || !entry.Resource)
		{
			return false;
		}
		entry.Resource.reset();
		if (entry.Generation != (std::numeric_limits<std::uint32_t>::max)())
		{
			++entry.Generation;
			m_freeSlots.push_back(handle.Slot);
		}
		return true;
	}

	void SceneMeshResources::Clear() noexcept
	{
		m_entries.clear();
		m_freeSlots.clear();
	}
}
