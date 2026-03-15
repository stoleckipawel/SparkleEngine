#pragma once

#include <cstdint>

enum class FrameGraphPassFlags : std::uint8_t
{
	None = 0,
	Raster = 1 << 0,
};

constexpr FrameGraphPassFlags operator|(FrameGraphPassFlags lhs, FrameGraphPassFlags rhs) noexcept
{
	return static_cast<FrameGraphPassFlags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr FrameGraphPassFlags operator&(FrameGraphPassFlags lhs, FrameGraphPassFlags rhs) noexcept
{
	return static_cast<FrameGraphPassFlags>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

constexpr FrameGraphPassFlags& operator|=(FrameGraphPassFlags& lhs, FrameGraphPassFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyPassFlags(FrameGraphPassFlags value, FrameGraphPassFlags flags) noexcept
{
	return static_cast<std::uint8_t>(value & flags) != 0;
}

constexpr FrameGraphPassFlags GetFrameGraphPassKindMask() noexcept
{
	return FrameGraphPassFlags::Raster;
}

constexpr FrameGraphPassFlags GetFrameGraphPassKind(FrameGraphPassFlags flags) noexcept
{
	return flags & GetFrameGraphPassKindMask();
}

constexpr bool HasExactlyOnePassKind(FrameGraphPassFlags flags) noexcept
{
	return GetFrameGraphPassKind(flags) == FrameGraphPassFlags::Raster;
}

constexpr const char* FrameGraphPassKindToString(FrameGraphPassFlags flags) noexcept
{
	switch (GetFrameGraphPassKind(flags))
	{
		case FrameGraphPassFlags::Raster:
			return "Raster";
		default:
			return "None";
	}
}

constexpr const char* FrameGraphPassFlagToString(FrameGraphPassFlags flag) noexcept
{
	switch (flag)
	{
		case FrameGraphPassFlags::Raster:
			return "Raster";
		default:
			return "None";
	}
}