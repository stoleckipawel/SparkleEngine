#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"

#include <cstdint>
#include <dxgiformat.h>
#include <filesystem>
#include <string>

namespace Engine::AssetAuthoring
{
	struct CookedTextureAssetBuild
	{
		Engine::Assets::CookedAssetId assetId = Engine::Assets::InvalidCookedAssetId;
		std::filesystem::path sourcePath;
		bool isSrgb = false;
	};

	class KtxTextureCooker final
	{
	  public:
		static bool BuildTextureAsset(
		    const std::filesystem::path& sourceTexturePath,
		    Engine::Assets::CookedTextureSemantic semantic,
		    CookedTextureAssetBuild& outTextureAsset,
		    std::string& outErrorMessage);
		static std::filesystem::path BuildTextureAssetPath(Engine::Assets::CookedAssetId textureAssetId);

		bool Cook(const CookedTextureAssetBuild& textureAsset, std::string& outErrorMessage) const;

	  private:
		enum class TextureColorSpace : std::uint8_t
		{
			Linear = 0,
			Srgb = 1,
		};

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
		static bool ResolveVkFormat(
		    DXGI_FORMAT dxgiFormat,
		    TextureColorSpace colorSpace,
		    std::uint32_t& outVkFormat,
		    std::string& outErrorMessage);
	};
}