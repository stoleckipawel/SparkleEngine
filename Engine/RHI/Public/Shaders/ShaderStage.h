#pragma once

#include <cstddef>
#include <cstdint>

enum class ShaderStage : std::uint8_t
{
	Vertex,
	Pixel,
	Geometry,
	Hull,
	Domain,
	Compute,
	Count
};

enum class ShaderStageMask : std::uint8_t
{
	None = 0,
	Vertex = 1 << 0,
	Pixel = 1 << 1,
	Geometry = 1 << 2,
	Hull = 1 << 3,
	Domain = 1 << 4,
	Compute = 1 << 5,
	AllGraphics = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
	All = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5),
};

constexpr ShaderStageMask operator|(ShaderStageMask lhs, ShaderStageMask rhs) noexcept
{
	return static_cast<ShaderStageMask>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr ShaderStageMask operator&(ShaderStageMask lhs, ShaderStageMask rhs) noexcept
{
	return static_cast<ShaderStageMask>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

constexpr ShaderStageMask& operator|=(ShaderStageMask& lhs, ShaderStageMask rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyShaderStageMask(ShaderStageMask value, ShaderStageMask flags) noexcept
{
	return static_cast<std::uint8_t>(value & flags) != 0;
}

constexpr ShaderStageMask ToShaderStageMask(ShaderStage stage) noexcept
{
	switch (stage)
	{
		case ShaderStage::Vertex:
			return ShaderStageMask::Vertex;
		case ShaderStage::Pixel:
			return ShaderStageMask::Pixel;
		case ShaderStage::Geometry:
			return ShaderStageMask::Geometry;
		case ShaderStage::Hull:
			return ShaderStageMask::Hull;
		case ShaderStage::Domain:
			return ShaderStageMask::Domain;
		case ShaderStage::Compute:
			return ShaderStageMask::Compute;
		case ShaderStage::Count:
		default:
			return ShaderStageMask::None;
	}
}

inline const char* GetShaderStagePrefix(ShaderStage stage)
{
	static constexpr const char* kPrefixes[] = {"vs", "ps", "gs", "hs", "ds", "cs"};
	static_assert(
	    (sizeof(kPrefixes) / sizeof(kPrefixes[0])) == static_cast<std::size_t>(ShaderStage::Count),
	    "Shader stage prefix table must match ShaderStage.");
	return kPrefixes[static_cast<std::size_t>(stage)];
}