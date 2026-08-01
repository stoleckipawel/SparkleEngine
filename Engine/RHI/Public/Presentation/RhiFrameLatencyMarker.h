#pragma once

#include <cstdint>

enum class ERhiFrameLatencyMarker : std::uint8_t
{
	SimulationStart = 0,
	SimulationEnd,
	RenderSubmitStart,
	RenderSubmitEnd,
	PresentStart,
	PresentEnd,
};
