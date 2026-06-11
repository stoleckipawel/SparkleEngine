#pragma once

#include <cstdint>

enum class EFrameGraphPassFlags : std::uint8_t
{
	None = 0,
	Raster = 1 << 0,
	Compute = 1 << 1,
	Transfer = 1 << 2,
	ExternalProvider = 1 << 3,
};

constexpr EFrameGraphPassFlags operator|(EFrameGraphPassFlags lhs, EFrameGraphPassFlags rhs) noexcept
{
	return static_cast<EFrameGraphPassFlags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr EFrameGraphPassFlags operator&(EFrameGraphPassFlags lhs, EFrameGraphPassFlags rhs) noexcept
{
	return static_cast<EFrameGraphPassFlags>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

constexpr EFrameGraphPassFlags& operator|=(EFrameGraphPassFlags& lhs, EFrameGraphPassFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyPassFlags(EFrameGraphPassFlags value, EFrameGraphPassFlags flags) noexcept
{
	return static_cast<std::uint8_t>(value & flags) != 0;
}

constexpr EFrameGraphPassFlags GetFrameGraphPassKindMask() noexcept
{
	return EFrameGraphPassFlags::Raster | EFrameGraphPassFlags::Compute | EFrameGraphPassFlags::Transfer |
	       EFrameGraphPassFlags::ExternalProvider;
}

constexpr EFrameGraphPassFlags GetFrameGraphPassKind(EFrameGraphPassFlags flags) noexcept
{
	return flags & GetFrameGraphPassKindMask();
}

constexpr bool HasExactlyOnePassKind(EFrameGraphPassFlags flags) noexcept
{
	const EFrameGraphPassFlags kind = GetFrameGraphPassKind(flags);
	return kind == EFrameGraphPassFlags::Raster || kind == EFrameGraphPassFlags::Compute || kind == EFrameGraphPassFlags::Transfer ||
	       kind == EFrameGraphPassFlags::ExternalProvider;
}

constexpr const char* FrameGraphPassKindToString(EFrameGraphPassFlags flags) noexcept
{
	switch (GetFrameGraphPassKind(flags))
	{
		case EFrameGraphPassFlags::Raster:
			return "Raster";
		case EFrameGraphPassFlags::Compute:
			return "Compute";
		case EFrameGraphPassFlags::Transfer:
			return "Transfer";
		case EFrameGraphPassFlags::ExternalProvider:
			return "ExternalProvider";
		default:
			return "None";
	}
}

constexpr const char* FrameGraphPassFlagToString(EFrameGraphPassFlags flag) noexcept
{
	switch (flag)
	{
		case EFrameGraphPassFlags::Raster:
			return "Raster";
		case EFrameGraphPassFlags::Compute:
			return "Compute";
		case EFrameGraphPassFlags::Transfer:
			return "Transfer";
		case EFrameGraphPassFlags::ExternalProvider:
			return "ExternalProvider";
		default:
			return "None";
	}
}
