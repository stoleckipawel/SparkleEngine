#pragma once

#include <cstdint>

namespace ECS
{
	class EntityRegistry;
	template <typename... AccessSpecs> class Query;

	class StructureFrozenEpoch final
	{
	public:
		StructureFrozenEpoch() noexcept = default;
		~StructureFrozenEpoch();

		StructureFrozenEpoch(const StructureFrozenEpoch&) = delete;
		StructureFrozenEpoch& operator=(const StructureFrozenEpoch&) = delete;
		StructureFrozenEpoch(StructureFrozenEpoch&& other) noexcept;
		StructureFrozenEpoch& operator=(StructureFrozenEpoch&& other) noexcept;

		bool IsValid() const noexcept { return m_registry != nullptr && m_generation != 0; }

	private:
		friend class EntityRegistry;
		template <typename... AccessSpecs> friend class Query;

		StructureFrozenEpoch(EntityRegistry& registry, std::uint64_t generation) noexcept :
		    m_registry(&registry),
		    m_generation(generation)
		{
		}

		void Release() noexcept;

		EntityRegistry* m_registry = nullptr;
		std::uint64_t m_generation = 0;
	};
}
