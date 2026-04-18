#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Engine::AssetAuthoring
{
	using TextureAssetId = std::uint64_t;

	inline constexpr TextureAssetId InvalidTextureAssetId = 0;

	enum class TextureColorSpace : std::uint8_t
	{
		Linear = 0,
		Srgb = 1,
	};

	struct TextureCookRequest final
	{
		TextureAssetId assetId = InvalidTextureAssetId;
		std::filesystem::path sourcePath;
		std::filesystem::path outputPath;
		TextureColorSpace colorSpace = TextureColorSpace::Linear;

		bool IsValid() const noexcept { return assetId != InvalidTextureAssetId && !sourcePath.empty() && !outputPath.empty(); }
		bool IsSrgb() const noexcept { return colorSpace == TextureColorSpace::Srgb; }
		explicit operator bool() const noexcept { return IsValid(); }
	};

	const char* GetTextureColorSpaceName(TextureColorSpace colorSpace) noexcept;

	bool WriteTextureCookRequestList(
	    const std::filesystem::path& outputPath,
	    const std::vector<TextureCookRequest>& requests,
	    std::string& outErrorMessage);

	bool LoadTextureCookRequestList(
	    const std::filesystem::path& inputPath,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
}