#pragma once

#include "../RendererAPI.h"

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
	Cubemap,

	Count
};

struct DefaultTextureDesc
{
	const char* name = "Unknown";
	const char* path = "";
};

namespace DefaultTextures
{
	SPARKLE_RENDERER_API const DefaultTextureDesc& GetDesc(DefaultTexture type);
	SPARKLE_RENDERER_API const char* GetName(DefaultTexture type);
	SPARKLE_RENDERER_API std::filesystem::path GetPath(DefaultTexture type);
}  // namespace DefaultTextures