#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace AssetAuthoring
{
	using TextureAssetId = std::uint64_t;

	inline constexpr TextureAssetId InvalidTextureAssetId = 0;

	enum class TextureColorSpace : std::uint8_t
	{
		Linear = 0,
		Srgb = 1,
	};

		enum class TextureMipPolicy : std::uint8_t
		{
			Generate = 0,
			PreserveExisting = 1,
			NoMips = 2,
		};

		enum class TextureMipFilter : std::uint8_t
		{
			Regular = 0,
			Kaiser = 1,
			NormalAware = 2,
			Angular = 3,
		};

		enum class TextureColorProcessingPolicy : std::uint8_t
		{
			Linear = 0,
			SrgbLinearize = 1,
		};

		enum class TextureCompressionFamilyPreference : std::uint8_t
		{
			None = 0,
			Color = 1,
			Masks = 2,
			NormalMap = 3,
			HdrColor = 4,
			CubeColor = 5,
		};

		enum class TextureDimension : std::uint8_t
		{
			Texture2D = 0,
			TextureCube = 1,
		};

	struct TextureCookRequest final
	{
		TextureAssetId assetId = InvalidTextureAssetId;
		std::filesystem::path sourcePath;
		std::filesystem::path outputPath;
		TextureColorSpace colorSpace = TextureColorSpace::Linear;
			TextureMipPolicy mipPolicy = TextureMipPolicy::Generate;
			TextureMipFilter mipFilter = TextureMipFilter::Regular;
			TextureColorProcessingPolicy colorProcessingPolicy = TextureColorProcessingPolicy::Linear;
			TextureCompressionFamilyPreference compressionFamilyPreference = TextureCompressionFamilyPreference::None;
			TextureDimension dimension = TextureDimension::Texture2D;

		bool IsValid() const noexcept { return assetId != InvalidTextureAssetId && !sourcePath.empty() && !outputPath.empty(); }
		bool IsSrgb() const noexcept { return colorSpace == TextureColorSpace::Srgb; }
			bool IsCube() const noexcept { return dimension == TextureDimension::TextureCube; }
		explicit operator bool() const noexcept { return IsValid(); }
	};

	const char* GetTextureColorSpaceName(TextureColorSpace colorSpace) noexcept;
		const char* GetTextureMipPolicyName(TextureMipPolicy mipPolicy) noexcept;
		const char* GetTextureMipFilterName(TextureMipFilter mipFilter) noexcept;
		const char* GetTextureColorProcessingPolicyName(TextureColorProcessingPolicy colorProcessingPolicy) noexcept;
		const char* GetTextureCompressionFamilyPreferenceName(TextureCompressionFamilyPreference compressionFamilyPreference) noexcept;
		const char* GetTextureDimensionName(TextureDimension dimension) noexcept;

	bool WriteTextureCookRequestList(
	    const std::filesystem::path& outputPath,
	    const std::vector<TextureCookRequest>& requests,
	    std::string& outErrorMessage);

	bool LoadTextureCookRequestList(
	    const std::filesystem::path& inputPath,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
}