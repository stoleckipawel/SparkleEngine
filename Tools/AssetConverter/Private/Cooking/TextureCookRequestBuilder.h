#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>

namespace AssetAuthoring
{
	class TextureCookRequestBuilder final
	{
	  public:
		static bool Build(
		    const std::filesystem::path& sourceTexturePath,
		    Assets::CookedTextureSemantic semantic,
		    TextureCookRequest& outRequest,
		    std::string& outErrorMessage);

	  private:
		static TextureColorSpace ResolveColorSpace(Assets::CookedTextureSemantic semantic) noexcept;
		static TextureMipPolicy ResolveMipPolicy(Assets::CookedTextureSemantic semantic) noexcept;
		static TextureMipFilter ResolveMipFilter(Assets::CookedTextureSemantic semantic) noexcept;
		static TextureColorProcessingPolicy ResolveColorProcessingPolicy(Assets::CookedTextureSemantic semantic) noexcept;
		static TextureCompressionFamilyPreference ResolveCompressionFamilyPreference(Assets::CookedTextureSemantic semantic) noexcept;
		static TextureDimension ResolveTextureDimension(Assets::CookedTextureSemantic semantic) noexcept;
		static bool NormalizeSourceTexturePath(
		    const std::filesystem::path& sourceTexturePath,
		    std::filesystem::path& outNormalizedSourceTexturePath,
		    std::string& outErrorMessage);
		static bool BuildTextureSourceKey(
		    const TextureCookRequest& request,
		    std::string& outTextureSourceKey,
		    std::string& outErrorMessage);
	};
}