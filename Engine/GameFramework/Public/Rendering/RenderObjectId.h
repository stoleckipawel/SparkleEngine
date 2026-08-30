#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <compare>
#include <cstdint>
#include <limits>

class SPARKLE_ENGINE_API RenderObjectId final
{
public:
	using Value = std::uint32_t;
	static constexpr Value InvalidValue = (std::numeric_limits<Value>::max)();
	constexpr RenderObjectId() noexcept = default;
	static constexpr RenderObjectId Invalid() noexcept { return {}; }
	static constexpr RenderObjectId FromParts(Value value, std::uint32_t generation) noexcept { return RenderObjectId(value, generation); }
	constexpr bool IsValid() const noexcept { return m_value != InvalidValue && m_generation != 0; }
	constexpr Value GetValue() const noexcept { return m_value; }
	constexpr std::uint32_t GetGeneration() const noexcept { return m_generation; }
	constexpr auto operator<=>(const RenderObjectId&) const noexcept = default;

private:
	constexpr RenderObjectId(Value value, std::uint32_t generation) noexcept :
	    m_value(value),
	    m_generation(generation)
	{
	}
	Value m_value = InvalidValue;
	std::uint32_t m_generation = 0;
};
