#pragma once

#include <cstdint>

enum class DispatchMode : std::uint8_t
{
	Immediate,  // Fire synchronously in WndProc context
	Deferred,   // Queue for processing in BeginFrame
};
