#pragma once

#include <cstdint>

namespace Pixel
{
	// sRGB <-> Linear conversion for byte channels
	// Converts between normalized float [0.0, 1.0] and sRGB-encoded byte [0, 255]
	std::uint8_t EncodeByteChannel(float linearValue, bool applySrgb) noexcept;
	float DecodeByteChannel(std::uint8_t byteValue, bool applySrgb) noexcept;
}
