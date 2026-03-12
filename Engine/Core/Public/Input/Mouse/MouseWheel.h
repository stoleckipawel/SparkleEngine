#pragma once

#include <cstdint>

enum class MouseWheelAxis : std::uint8_t
{
	Vertical,
	Horizontal,

	Count
};

struct MouseWheelState
{
	float Delta = 0.0f;

	float Accumulated = 0.0f;

	constexpr void ResetDelta() noexcept { Delta = 0.0f; }

	constexpr void ResetAccumulated() noexcept { Accumulated = 0.0f; }

	constexpr void Reset() noexcept
	{
		Delta = 0.0f;
		Accumulated = 0.0f;
	}

	constexpr void AddDelta(float wheelDelta) noexcept
	{
		Delta += wheelDelta;
		Accumulated += wheelDelta;
	}
};

struct MouseWheel
{
	MouseWheelState Vertical;
	MouseWheelState Horizontal;

	constexpr void ResetDeltas() noexcept
	{
		Vertical.ResetDelta();
		Horizontal.ResetDelta();
	}

	constexpr void Reset() noexcept
	{
		Vertical.Reset();
		Horizontal.Reset();
	}

	constexpr MouseWheelState& operator[](MouseWheelAxis axis) noexcept
	{
		return axis == MouseWheelAxis::Horizontal ? Horizontal : Vertical;
	}

	constexpr const MouseWheelState& operator[](MouseWheelAxis axis) const noexcept
	{
		return axis == MouseWheelAxis::Horizontal ? Horizontal : Vertical;
	}
};
