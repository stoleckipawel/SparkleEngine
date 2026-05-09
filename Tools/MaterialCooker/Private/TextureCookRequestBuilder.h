#pragma once

#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>

class TextureCookRequestBuilder final
{
public:
	static bool Build(
	    const std::filesystem::path& sourceTexturePath,
	    TextureGroup textureGroup,
	    TextureCookRequest& outRequest,
	    std::string& outErrorMessage,
	    TextureChannelMask channelMask = TextureChannelMask::Rgba);

private:
	static TextureColorSpace ResolveColorSpace(TextureGroup textureGroup) noexcept;
	static TextureMipPolicy ResolveMipPolicy(TextureGroup textureGroup) noexcept;
	static TextureMipFilter ResolveMipFilter(TextureGroup textureGroup) noexcept;
	static TextureColorProcessingPolicy ResolveColorProcessingPolicy(TextureGroup textureGroup) noexcept;
	static TextureDimension ResolveTextureDimension(TextureGroup textureGroup) noexcept;
	static bool NormalizeSourceTexturePath(
	    const std::filesystem::path& sourceTexturePath,
	    std::filesystem::path& outNormalizedSourceTexturePath,
	    std::string& outErrorMessage);
	static bool BuildTextureSourceKey(
	    const TextureCookRequest& request,
	    std::string& outTextureSourceKey,
	    std::string& outErrorMessage);
};

