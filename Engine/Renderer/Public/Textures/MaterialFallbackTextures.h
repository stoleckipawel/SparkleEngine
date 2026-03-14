#pragma once

#include "Renderer/Public/Textures/DefaultTextures.h"
#include "Renderer/Public/RendererAPI.h"

#include <cstdint>

enum class MaterialFallbackTexture : std::uint8_t
{
	Albedo,
	Normal,
	MetallicRoughness,
	Occlusion,
	Emissive,

	Count
};

struct MaterialFallbackTextureDesc
{
	const char* name = "Unknown";
	DefaultTexture defaultTexture = DefaultTexture::White;
};

namespace MaterialFallbackTextures
{
	SPARKLE_RENDERER_API const MaterialFallbackTextureDesc& GetDesc(MaterialFallbackTexture type);
	SPARKLE_RENDERER_API const char* GetName(MaterialFallbackTexture type);
	SPARKLE_RENDERER_API DefaultTexture GetDefaultTexture(MaterialFallbackTexture type);
}  // namespace MaterialFallbackTextures