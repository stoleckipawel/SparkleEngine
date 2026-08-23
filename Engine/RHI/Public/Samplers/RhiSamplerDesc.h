#pragma once

#include <cstdint>

enum class RhiSamplerMinMagFilter : std::uint8_t
{
	Point = 0,
	Linear,
};

enum class RhiSamplerMipFilter : std::uint8_t
{
	None = 0,
	Point,
	Linear,
};

enum class RhiSamplerAddressMode : std::uint8_t
{
	Wrap = 0,
	Clamp,
	Mirror,
};

struct RhiSamplerAddressModes
{
	RhiSamplerAddressMode U = RhiSamplerAddressMode::Wrap;
	RhiSamplerAddressMode V = RhiSamplerAddressMode::Wrap;
	RhiSamplerAddressMode W = RhiSamplerAddressMode::Wrap;

	bool operator==(const RhiSamplerAddressModes&) const noexcept = default;
};

constexpr RhiSamplerAddressModes MakeRhiSamplerAddressModes(RhiSamplerAddressMode addressMode) noexcept
{
	return RhiSamplerAddressModes{addressMode, addressMode, addressMode};
}

enum class RhiSamplerAnisotropy : std::uint8_t
{
	X1 = 1,
	X2 = 2,
	X4 = 4,
	X8 = 8,
	X16 = 16,
};

struct RhiSamplerDesc
{
	RhiSamplerMinMagFilter MinMagFilter = RhiSamplerMinMagFilter::Linear;
	RhiSamplerMipFilter MipFilter = RhiSamplerMipFilter::Linear;
	RhiSamplerAddressModes Address = {};
	RhiSamplerAnisotropy MaxAnisotropy = RhiSamplerAnisotropy::X1;

	bool operator==(const RhiSamplerDesc&) const noexcept = default;
};
