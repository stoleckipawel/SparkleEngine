#include "PCH.h"
#include "Renderer/Public/Textures/DefaultTextures.h"

namespace
{
	constexpr DefaultTextureDesc kDefaultTextureDescs[] = {
	    {"Checkerboard", "Defaults/default_checkerboard.png"},
	    {"White", "Defaults/default_white.png"},
	    {"Black", "Defaults/default_black.png"},
	    {"Red", "Defaults/default_red.png"},
	    {"Green", "Defaults/default_green.png"},
	    {"Blue", "Defaults/default_blue.png"},
	    {"Normal", "Defaults/default_normal.png"},
	    {"Cubemap", "Defaults/default_cubemap.png"}};

	constexpr DefaultTextureDesc kUnknownDefaultTextureDesc{};
}  // namespace

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