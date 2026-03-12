// =============================================================================
// MouseWheel.h — Mouse Wheel State and Delta Tracking
// =============================================================================
//
// Tracks mouse wheel rotation for both vertical and horizontal wheels.
// Supports high-precision wheels with fractional delta values.
//
// USAGE:
//   // In your controller
//   float delta = inputState.GetWheelDelta();
//   m_moveSpeedMultiplier += delta * 0.1f;
//   m_moveSpeedMultiplier = std::clamp(m_moveSpeedMultiplier, 0.1f, 10.0f);
//
// DESIGN:
//   - Delta is normalized: +1.0 = one "notch" up/forward, -1.0 = one notch down/back
//   - High-precision mice may report fractional values
//   - Horizontal wheel (tilt) tracked separately
//   - Accumulated value available for total scroll distance
//
// =============================================================================

#pragma once

#include <cstdint>

// =============================================================================
// MouseWheelAxis — Identifies vertical vs horizontal wheel
// =============================================================================

enum class MouseWheelAxis : std::uint8_t
{
	Vertical,    // Standard scroll wheel (up/down)
	Horizontal,  // Tilt wheel or horizontal scroll (left/right)

	Count
};

// =============================================================================
// MouseWheelState — Tracks wheel rotation per frame
// =============================================================================

/// Holds per-frame wheel delta and accumulated scroll distance.
/// Delta is reset each frame; accumulated persists until manually reset.
struct MouseWheelState
{
	/// Wheel movement this frame. Positive = up/right, Negative = down/left.
	/// One "notch" on a standard wheel ≈ 1.0 (WHEEL_DELTA normalized).
	float Delta = 0.0f;

	/// Total accumulated scroll since last reset. Useful for:
	/// - Smooth zoom (accumulate then apply with damping)
	/// - Discrete steps (check when accumulated crosses threshold)
	float Accumulated = 0.0f;

	/// Resets per-frame delta (call at frame start).
	constexpr void ResetDelta() noexcept { Delta = 0.0f; }

	/// Resets accumulated scroll (call when user releases interaction).
	constexpr void ResetAccumulated() noexcept { Accumulated = 0.0f; }

	/// Resets both delta and accumulated.
	constexpr void Reset() noexcept
	{
		Delta = 0.0f;
		Accumulated = 0.0f;
	}

	/// Adds wheel movement (called by input backend).
	constexpr void AddDelta(float wheelDelta) noexcept
	{
		Delta += wheelDelta;
		Accumulated += wheelDelta;
	}
};

// =============================================================================
// MouseWheel — Combined vertical and horizontal wheel state
// =============================================================================

/// Complete mouse wheel state for both axes.
struct MouseWheel
{
	MouseWheelState Vertical;    ///< Standard scroll wheel
	MouseWheelState Horizontal;  ///< Tilt/horizontal wheel

	/// Resets per-frame deltas for both axes.
	constexpr void ResetDeltas() noexcept
	{
		Vertical.ResetDelta();
		Horizontal.ResetDelta();
	}

	/// Resets all state for both axes.
	constexpr void Reset() noexcept
	{
		Vertical.Reset();
		Horizontal.Reset();
	}

	/// Gets state by axis enum.
	constexpr MouseWheelState& operator[](MouseWheelAxis axis) noexcept
	{
		return axis == MouseWheelAxis::Horizontal ? Horizontal : Vertical;
	}

	constexpr const MouseWheelState& operator[](MouseWheelAxis axis) const noexcept
	{
		return axis == MouseWheelAxis::Horizontal ? Horizontal : Vertical;
	}
};
