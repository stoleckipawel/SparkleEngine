#pragma once

#include <cstdint>
#include <DirectXMath.h>

struct MousePosition
{
	std::int32_t X = 0;
	std::int32_t Y = 0;

	float NormalizedX = 0.0f;
	float NormalizedY = 0.0f;

	constexpr DirectX::XMINT2 AsInt() const noexcept { return DirectX::XMINT2{X, Y}; }

	constexpr DirectX::XMFLOAT2 AsFloat() const noexcept
	{
		return DirectX::XMFLOAT2{static_cast<float>(X), static_cast<float>(Y)};
	}

	constexpr DirectX::XMFLOAT2 AsNormalized() const noexcept { return DirectX::XMFLOAT2{NormalizedX, NormalizedY}; }

	constexpr DirectX::XMFLOAT2 AsNDC() const noexcept
	{
		return DirectX::XMFLOAT2{
		    NormalizedX * 2.0f - 1.0f,
		    1.0f - NormalizedY * 2.0f
		};
	}

	static constexpr MousePosition FromPixels(
	    std::int32_t x,
	    std::int32_t y,
	    std::uint32_t windowWidth,
	    std::uint32_t windowHeight) noexcept
	{
		MousePosition pos;
		pos.X = x;
		pos.Y = y;
		pos.NormalizedX = windowWidth > 0 ? static_cast<float>(x) / static_cast<float>(windowWidth) : 0.0f;
		pos.NormalizedY = windowHeight > 0 ? static_cast<float>(y) / static_cast<float>(windowHeight) : 0.0f;
		return pos;
	}
};

struct MouseDelta
{
	float X = 0.0f;
	float Y = 0.0f;

	constexpr DirectX::XMFLOAT2 AsFloat() const noexcept { return DirectX::XMFLOAT2{X, Y}; }

	constexpr DirectX::XMFLOAT2 AsFloatYUp() const noexcept { return DirectX::XMFLOAT2{X, -Y}; }

	constexpr void Accumulate(float dx, float dy) noexcept
	{
		X += dx;
		Y += dy;
	}

	constexpr void Reset() noexcept
	{
		X = 0.0f;
		Y = 0.0f;
	}
};
