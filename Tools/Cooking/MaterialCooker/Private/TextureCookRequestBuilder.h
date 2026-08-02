#pragma once

#include "TextureCookRequestList.h"

#include <filesystem>
#include <optional>
#include <string_view>

class TextureCookRequestBuilder final
{
public:
	static TextureCookRequest Build(
	    const std::filesystem::path& sourceTexturePath,
	    TextureGroup textureGroup,
	    TextureChannelMask channelMask = TextureChannelMask::Rgba);

private:
	static std::string BuildTextureSourceKeyForRoot(
	    std::string_view rootName,
	    const TextureCookRequest& request,
	    const std::filesystem::path& relativePath);
	static std::optional<std::filesystem::path> BuildTextureOutputPathForRoot(
	    const std::filesystem::path& sourceTexturePath,
	    const std::filesystem::path& sourceRoot,
	    std::string_view cookedDirectory,
	    std::string_view variantSuffix);
	static TextureColorSpace ResolveColorSpace(TextureGroup textureGroup) noexcept;
	static TextureMipPolicy ResolveMipPolicy(const std::filesystem::path& sourceTexturePath, TextureGroup textureGroup) noexcept;
	static TextureMipFilter ResolveMipFilter(TextureGroup textureGroup) noexcept;
	static TextureColorProcessingPolicy ResolveColorProcessingPolicy(TextureGroup textureGroup) noexcept;
	static TextureDimension ResolveTextureDimension(TextureGroup textureGroup) noexcept;
	static std::filesystem::path NormalizeSourceTexturePath(const std::filesystem::path& sourceTexturePath);
	static std::string BuildTextureSourceKey(const TextureCookRequest& request);
	static std::filesystem::path BuildTextureOutputPath(const TextureCookRequest& request);
	static std::string BuildTextureVariantSuffix(const TextureCookRequest& request);
};
