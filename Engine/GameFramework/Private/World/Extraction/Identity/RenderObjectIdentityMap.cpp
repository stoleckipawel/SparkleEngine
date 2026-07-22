#include "PCH.h"
#include "World/Extraction/Identity/RenderObjectIdentityMap.h"

#include <limits>

namespace ECS
{
	void RenderObjectIdentityMap::BeginScene() noexcept
	{
		m_objects.clear();
		m_nextValue = 0;
		if (m_generation == (std::numeric_limits<std::uint32_t>::max)()) m_generation = 0;
		++m_generation;
	}

	RenderObjectId RenderObjectIdentityMap::Resolve(EntityId entity)
	{
		const auto existing = m_objects.find(entity);
		if (existing != m_objects.end()) return existing->second;
		if (m_nextValue == (std::numeric_limits<std::uint32_t>::max)()) return {};

		const RenderObjectId object = RenderObjectId::FromParts(m_nextValue++, m_generation);
		m_objects.emplace(entity, object);
		return object;
	}
}
