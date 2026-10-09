#pragma once

#include <cstdint>
#include <span>

// Immutable square, top-down, unpremultiplied RGBA8 artwork, extent in [1,512].
// Descriptor and pixels outlive every service using them; catalogs are static.
struct EditorIconAsset final
{
	const int Extent;
	const std::span<const std::uint8_t> Pixels;
};
