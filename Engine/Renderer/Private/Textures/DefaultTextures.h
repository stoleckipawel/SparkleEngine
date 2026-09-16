#pragma once

#include <cstdint>
#include <filesystem>

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

namespace DefaultTextures
{
	const DefaultTextureDesc& GetDesc(DefaultTexture type);
	const char* GetName(DefaultTexture type);
	std::filesystem::path GetPath(DefaultTexture type);
}
