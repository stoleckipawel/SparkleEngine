// ============================================================================
// DefaultTextures.cpp
// ============================================================================

#include "PCH.h"
#include "Renderer/Public/Textures/DefaultTextures.h"

namespace
{
	constexpr DefaultTextureDesc kDefaultTextureDescs[] = {
	    {"White", "Defaults/white.png"},
	    {"Black", "Defaults/black.jpg"},
	    {"FlatNormal", "Defaults/flat_normal.jpg"},
	    {"DefaultMetallicRoughness", "Defaults/default_metallic_roughness.jpg"}};

	constexpr DefaultTextureDesc kUnknownDefaultTextureDesc{};
}

const DefaultTextureDesc& DefaultTextures::GetDesc(DefaultTexture type)
{
	const auto index = static_cast<std::size_t>(type);
	return index < static_cast<std::size_t>(DefaultTexture::Count) ? kDefaultTextureDescs[index] : kUnknownDefaultTextureDesc;
}

const char* DefaultTextures::GetName(DefaultTexture type)
{
	return GetDesc(type).name;
}

std::filesystem::path DefaultTextures::GetPath(DefaultTexture type)
{
	return GetDesc(type).path;
}