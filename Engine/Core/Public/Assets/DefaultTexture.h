#pragma once

#include <cstdint>

enum class DefaultTexture : std::uint8_t
{
	Checkerboard,
	White,
	Black,
	Red,
	Green,
	Blue,
	Normal,
	Sky,

	Count
};

struct DefaultTextureDesc
{
	const char* name = "Unknown";
	const char* path = "";
};

inline constexpr DefaultTextureDesc DefaultTextureDescs[] = {
    {"Checkerboard", "Defaults/default_checkerboard.stex"},
    {"White", "Defaults/default_white.stex"},
    {"Black", "Defaults/default_black.stex"},
    {"Red", "Defaults/default_red.stex"},
    {"Green", "Defaults/default_green.stex"},
    {"Blue", "Defaults/default_blue.stex"},
    {"Normal", "Defaults/default_normal.stex"},
    {"Sky", "Defaults/default_cubemap.stex"}};

static_assert(sizeof(DefaultTextureDescs) / sizeof(DefaultTextureDescs[0]) == static_cast<std::uint8_t>(DefaultTexture::Count));
