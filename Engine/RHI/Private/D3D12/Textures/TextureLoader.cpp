#include "PCH.h"

#include "D3D12/Textures/CookedTextureAssetLoader.h"
#include "D3D12/Textures/TextureLoader.h"

#include "Core/Public/Paths/PathUtils.h"

#include <format>

static const auto g_textureLoaderLogger = Logging::GetOrCreateLogger("RHI.Textures");

TextureLoadResult TextureLoader::Load(const std::filesystem::path& fileName)
{
	static const CookedTextureAssetLoader cookedTextureAssetLoader;

	const std::wstring extension = Paths::GetLowercaseExtension(fileName);
	if (cookedTextureAssetLoader.SupportsExtension(extension))
	{
		return cookedTextureAssetLoader.Load(fileName);
	}

	Diagnostics::Fail(
	    g_textureLoaderLogger,
	    __FILE__,
	    __LINE__,
	    std::format("TextureLoader: Runtime texture loading only accepts cooked Sparkle texture packages: '{}'", fileName.string()));
	return {};
}
