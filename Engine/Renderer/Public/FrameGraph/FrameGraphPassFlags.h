#pragma once

#include <cstdint>

enum class FrameGraphPassFlags : std::uint8_t
{
	None = 0,
	Raster = 1 << 0,
	Compute = 1 << 1,
	Transfer = 1 << 2,
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
	return FrameGraphPassFlags::Raster | FrameGraphPassFlags::Compute | FrameGraphPassFlags::Transfer;
}

constexpr FrameGraphPassFlags GetFrameGraphPassKind(FrameGraphPassFlags flags) noexcept
{
	return flags & GetFrameGraphPassKindMask();
}

constexpr bool HasExactlyOnePassKind(FrameGraphPassFlags flags) noexcept
{
	const FrameGraphPassFlags kind = GetFrameGraphPassKind(flags);
	return kind == FrameGraphPassFlags::Raster || kind == FrameGraphPassFlags::Compute || kind == FrameGraphPassFlags::Transfer;
}

constexpr const char* FrameGraphPassKindToString(FrameGraphPassFlags flags) noexcept
{
	switch (GetFrameGraphPassKind(flags))
	{
		case FrameGraphPassFlags::Raster:
			return "Raster";
		case FrameGraphPassFlags::Compute:
			return "Compute";
		case FrameGraphPassFlags::Transfer:
			return "Transfer";
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
		case FrameGraphPassFlags::Compute:
			return "Compute";
		case FrameGraphPassFlags::Transfer:
			return "Transfer";
		default:
			return "None";
	}
}