#include "PCH.h"

#include "D3D12/Textures/TextureLoader.h"

#include "Core/Public/Paths/PathUtils.h"
#include "Log.h"
#include "D3D12/Textures/DdsTextureLoader.h"
#include "D3D12/Textures/WicTextureLoader.h"

#include <array>
#include <format>

TextureLoadResult TextureLoader::Load(const std::filesystem::path& fileName)
{
	static const DdsTextureLoader ddsTextureLoader;
	static const WicTextureLoader wicTextureLoader;
	static const std::array<const TextureLoaderBackend*, 2> textureLoaderBackends = {&ddsTextureLoader, &wicTextureLoader};

	const std::wstring extension = Engine::Paths::GetLowercaseExtension(fileName);
	for (const TextureLoaderBackend* textureLoaderBackend : textureLoaderBackends)
	{
		if (textureLoaderBackend->SupportsExtension(extension))
		{
			return textureLoaderBackend->Load(fileName);
		}
	}

	LOG_FATAL(std::format("TextureLoader: No registered texture loader supports '{}'", fileName.string()));
	return textureLoaderBackends.back()->Load(fileName);
}
