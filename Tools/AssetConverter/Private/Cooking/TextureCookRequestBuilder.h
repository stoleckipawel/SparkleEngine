#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>

namespace Engine::AssetAuthoring
{
	class TextureCookRequestBuilder final
	{
	  public:
		static bool Build(
		    const std::filesystem::path& sourceTexturePath,
		    Engine::Assets::CookedTextureSemantic semantic,
		    TextureCookRequest& outRequest,
		    std::string& outErrorMessage);

	  private:
		static TextureColorSpace ResolveColorSpace(Engine::Assets::CookedTextureSemantic semantic) noexcept;
		static bool NormalizeSourceTexturePath(
		    const std::filesystem::path& sourceTexturePath,
		    std::filesystem::path& outNormalizedSourceTexturePath,
		    std::string& outErrorMessage);
		static bool BuildTextureSourceKey(
		    const std::filesystem::path& normalizedSourceTexturePath,
		    TextureColorSpace colorSpace,
		    std::string& outTextureSourceKey,
		    std::string& outErrorMessage);
	};
}