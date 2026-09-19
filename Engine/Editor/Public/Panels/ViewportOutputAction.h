#pragma once

#include <cstdint>

enum class ViewportOutputAction : std::uint8_t
{
	None = 0,
	CapturePresentation,
	SaveCurrentPrefix,
	SaveComplete,
	SaveCheckpoint,
};
