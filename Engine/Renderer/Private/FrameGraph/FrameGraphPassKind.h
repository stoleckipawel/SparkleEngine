#pragma once

#include <cstdint>

enum class EFrameGraphPassKind : std::uint8_t
{
	None,
	Raster,
	Compute,
	Transfer,
	ExternalProvider,
};

constexpr bool IsValidFrameGraphPassKind(EFrameGraphPassKind kind) noexcept
{
	return kind != EFrameGraphPassKind::None;
}

constexpr const char* FrameGraphPassKindToString(EFrameGraphPassKind kind) noexcept
{
	switch (kind)
	{
		case EFrameGraphPassKind::Raster:
			return "Raster";
		case EFrameGraphPassKind::Compute:
			return "Compute";
		case EFrameGraphPassKind::Transfer:
			return "Transfer";
		case EFrameGraphPassKind::ExternalProvider:
			return "ExternalProvider";
		default:
			return "None";
	}
}
