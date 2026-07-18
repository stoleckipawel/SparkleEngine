#include "PCH.h"
#include "World/ECS/StructureFrozenEpoch.h"

#include "World/ECS/EntityRegistry.h"

#include <utility>

namespace ECS
{
	StructureFrozenEpoch::~StructureFrozenEpoch()
	{
		Release();
	}

	StructureFrozenEpoch::StructureFrozenEpoch(StructureFrozenEpoch&& other) noexcept :
	    m_registry(std::exchange(other.m_registry, nullptr)), m_generation(std::exchange(other.m_generation, 0))
	{
	}

	StructureFrozenEpoch& StructureFrozenEpoch::operator=(StructureFrozenEpoch&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			m_registry = std::exchange(other.m_registry, nullptr);
			m_generation = std::exchange(other.m_generation, 0);
		}
		return *this;
	}

	void StructureFrozenEpoch::Release() noexcept
	{
		if (m_registry != nullptr)
		{
			m_registry->ReleaseFrozenEpoch(m_generation);
			m_registry = nullptr;
			m_generation = 0;
		}
	}
}
