// ============================================================================
// DefaultTextures.cpp
// ============================================================================

#include "PCH.h"
#include "Renderer/Public/Textures/DefaultTextures.h"

namespace
{
	constexpr DefaultTextureDesc kDefaultTextureDescs[] = {
	    {"White", "Sponza/white.png"},
	    {"Black", "DamagedHelmet/Default_emissive.jpg"},
	    {"FlatNormal", "DamagedHelmet/Default_normal.jpg"},
	    {"DefaultMetallicRoughness", "DamagedHelmet/Default_metalRoughness.jpg"}};

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