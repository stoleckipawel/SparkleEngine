#include "PCH.h"
#include "Renderer/Public/Resources/Textures/DefaultTextures.h"

class DefaultTexturesConstants final
{
public:
	static constexpr DefaultTextureDesc kDefaultTextureDescs[] = {
	    {"Checkerboard", "Defaults/default_checkerboard.stex"},
	    {"White", "Defaults/default_white.stex"},
	    {"Black", "Defaults/default_black.stex"},
	    {"Red", "Defaults/default_red.stex"},
	    {"Green", "Defaults/default_green.stex"},
	    {"Blue", "Defaults/default_blue.stex"},
	    {"Normal", "Defaults/default_normal.stex"},
	    {"Sky", "Defaults/default_cubemap.stex"}};
};

static const auto g_defaultTexturesLogger = Logging::GetOrCreateLogger("Renderer.DefaultTextures");

const DefaultTextureDesc& DefaultTextures::GetDesc(DefaultTexture type)
{
	const auto index = static_cast<std::size_t>(type);
	if (index >= static_cast<std::size_t>(DefaultTexture::Count))
	{
		Diagnostics::Fatal(g_defaultTexturesLogger, __FILE__, __LINE__, "Invalid default texture type.");
	}
	return DefaultTexturesConstants::kDefaultTextureDescs[index];
}

const char* DefaultTextures::GetName(DefaultTexture type)
{
	return GetDesc(type).name;
}

std::filesystem::path DefaultTextures::GetPath(DefaultTexture type)
{
	return GetDesc(type).path;
}
