#pragma once

#include <compare>
#include <cstdint>
#include <string_view>

namespace ECS
{
	struct ComponentSchemaId final
	{
		std::uint64_t Value = 0;

		constexpr bool IsValid() const noexcept { return Value != 0; }
		constexpr auto operator<=>(const ComponentSchemaId&) const noexcept = default;
	};

	struct ComponentSchema final
	{
		ComponentSchemaId Id;
		std::string_view Name;
	};

	constexpr ComponentSchemaId MakeComponentSchemaId(std::string_view canonicalName) noexcept
	{
		constexpr std::uint64_t OffsetBasis = 14695981039346656037ull;
		constexpr std::uint64_t Prime = 1099511628211ull;

		std::uint64_t hash = OffsetBasis;
		for (const char character : canonicalName)
		{
			hash ^= static_cast<std::uint8_t>(character);
			hash *= Prime;
		}
		return ComponentSchemaId{hash};
	}

	template <typename T> struct ComponentSchemaTraits;

	template <typename T> constexpr ComponentSchema GetComponentSchema() noexcept
	{
		return ComponentSchemaTraits<T>::Schema;
	}
}
