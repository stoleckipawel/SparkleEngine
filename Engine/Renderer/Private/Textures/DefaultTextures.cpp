#include "PCH.h"
#include "Textures/DefaultTextures.h"

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_defaultTexturesLogger, "Renderer.DefaultTextures");

const DefaultTextureDesc& DefaultTextures::GetDesc(DefaultTexture type)
{
	const auto index = static_cast<std::size_t>(type);
	if (index >= static_cast<std::size_t>(DefaultTexture::Count))
	{
		Diagnostics::Fatal(g_defaultTexturesLogger, __FILE__, __LINE__, "Invalid default texture type.");
	}
	return DefaultTextureDescs[index];
}

const char* DefaultTextures::GetName(DefaultTexture type)
{
	return GetDesc(type).name;
}

std::filesystem::path DefaultTextures::GetPath(DefaultTexture type)
{
	return GetDesc(type).path;
}
