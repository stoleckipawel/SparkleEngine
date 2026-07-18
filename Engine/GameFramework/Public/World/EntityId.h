#pragma once

#include <compare>
#include <cstdint>
#include <limits>

namespace ECS
{
	class EntityRegistry;
}

class EntityId final
{
  public:
	using Slot = std::uint32_t;
	using Generation = std::uint32_t;

	static constexpr Slot InvalidSlot = (std::numeric_limits<Slot>::max)();
	static constexpr Generation InvalidGeneration = 0;

	constexpr EntityId() noexcept = default;

	static constexpr EntityId Invalid() noexcept { return EntityId{}; }

	constexpr bool IsValid() const noexcept { return m_slot != InvalidSlot && m_generation != InvalidGeneration; }
	constexpr Slot GetSlot() const noexcept { return m_slot; }
	constexpr Generation GetGeneration() const noexcept { return m_generation; }

	constexpr auto operator<=>(const EntityId&) const noexcept = default;

  private:
	friend class ECS::EntityRegistry;

	constexpr EntityId(Slot slot, Generation generation) noexcept : m_slot(slot), m_generation(generation) {}

	Slot m_slot = InvalidSlot;
	Generation m_generation = InvalidGeneration;
};
