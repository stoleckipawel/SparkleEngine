#pragma once

#include <cstdint>
#include <DirectXMath.h>

// =============================================================================
// MousePosition — Multi-Format Position Data
// =============================================================================

/// Mouse position stored in multiple formats for convenience.
/// Screen pixels (integer) and normalized [0,1] (float) are both available.
struct MousePosition
{
	// -------------------------------------------------------------------------
	// Screen Pixel Coordinates
	// -------------------------------------------------------------------------

	std::int32_t X = 0;  ///< X position in screen pixels (left = 0)
	std::int32_t Y = 0;  ///< Y position in screen pixels (top = 0)

	// -------------------------------------------------------------------------
	// Normalized Coordinates [0, 1]
	// -------------------------------------------------------------------------

	float NormalizedX = 0.0f;  ///< X position normalized (0.0 = left, 1.0 = right)
	float NormalizedY = 0.0f;  ///< Y position normalized (0.0 = top, 1.0 = bottom)

	// -------------------------------------------------------------------------
	// Convenience Accessors
	// -------------------------------------------------------------------------

	/// Returns pixel position as XMINT2.
	constexpr DirectX::XMINT2 AsInt() const noexcept { return DirectX::XMINT2{X, Y}; }

	/// Returns pixel position as float vector.
	constexpr DirectX::XMFLOAT2 AsFloat() const noexcept
	{
		return DirectX::XMFLOAT2{static_cast<float>(X), static_cast<float>(Y)};
	}

	/// Returns normalized position [0,1].
	constexpr DirectX::XMFLOAT2 AsNormalized() const noexcept { return DirectX::XMFLOAT2{NormalizedX, NormalizedY}; }

	/// Returns normalized position in [-1,1] range (NDC-style, Y-up).
	/// Useful for screen-space ray casting.
	constexpr DirectX::XMFLOAT2 AsNDC() const noexcept
	{
		return DirectX::XMFLOAT2{
		    NormalizedX * 2.0f - 1.0f,  // [0,1] → [-1,1]
		    1.0f - NormalizedY * 2.0f   // [0,1] → [1,-1] (flip Y)
		};
	}

	// -------------------------------------------------------------------------
	// Factory
	// -------------------------------------------------------------------------

	/// Creates MousePosition from pixel coordinates and window dimensions.
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

// =============================================================================
// MouseDelta — Mouse Movement Delta
// =============================================================================

/// Represents mouse movement between frames or events.
struct MouseDelta
{
	float X = 0.0f;  ///< Horizontal movement in pixels (positive = right)
	float Y = 0.0f;  ///< Vertical movement in pixels (positive = down)

	/// Returns delta as XMFLOAT2.
	constexpr DirectX::XMFLOAT2 AsFloat() const noexcept { return DirectX::XMFLOAT2{X, Y}; }

	/// Returns delta with Y inverted (positive = up).
	constexpr DirectX::XMFLOAT2 AsFloatYUp() const noexcept { return DirectX::XMFLOAT2{X, -Y}; }

	/// Accumulates another delta into this one.
	constexpr void Accumulate(float dx, float dy) noexcept
	{
		X += dx;
		Y += dy;
	}

	/// Resets delta to zero.
	constexpr void Reset() noexcept
	{
		X = 0.0f;
		Y = 0.0f;
	}
};
