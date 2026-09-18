#pragma once

#include <cstdint>

enum class ReferencePathTracerOutputAction : std::uint8_t
{
	None = 0,
	SavePartial,
	SaveWhenComplete,
	SaveComplete,
	SaveCheckpoint,
};
