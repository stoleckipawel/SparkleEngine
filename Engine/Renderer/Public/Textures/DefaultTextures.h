// ============================================================================
// DefaultTextures.h
// ----------------------------------------------------------------------------
// Shared renderer default texture definitions.
// These are generic defaults, not material-slot-specific policies.
// ============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"

#include <cstdint>
#include <filesystem>

enum class DefaultTexture : std::uint8_t
{
	White,
	Black,
	FlatNormal,
	DefaultMetallicRoughness,

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
}