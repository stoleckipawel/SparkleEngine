#pragma once

#include "TextureCookRequestList.h"

#include <string_view>

namespace TextureCookPolicyCodec
{
	bool TryParseColorSpace(std::string_view value, TextureColorSpace& outColorSpace) noexcept;
	bool TryParseMipPolicy(std::string_view value, TextureMipPolicy& outMipPolicy) noexcept;
	bool TryParseMipFilter(std::string_view value, TextureMipFilter& outMipFilter) noexcept;
	bool TryParseColorProcessingPolicy(
	    std::string_view value, TextureColorProcessingPolicy& outColorProcessingPolicy) noexcept;
	bool TryParseTextureGroup(std::string_view value, TextureGroup& outTextureGroup) noexcept;
	bool TryParseDimension(std::string_view value, TextureDimension& outDimension) noexcept;
	bool TryParseChannelMask(std::string_view value, TextureChannelMask& outChannelMask) noexcept;
}
