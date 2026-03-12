#pragma once

#include <cstdint>

enum class DispatchMode : std::uint8_t
{
	Immediate,
	Deferred,
};
