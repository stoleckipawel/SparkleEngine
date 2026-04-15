#pragma once

#include "../RHIAPI.h"

#include <cstdint>

enum class PixelFormat : std::uint8_t
{
	Unknown = 0,
	R8G8B8A8_UNorm,
	B8G8R8A8_UNorm,
	D24_UNorm_S8_UInt,
	R32_Float,
};