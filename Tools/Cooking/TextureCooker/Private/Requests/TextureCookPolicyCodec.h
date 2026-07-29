#pragma once

#include "TextureCookRequestList.h"

#include <string_view>

namespace TextureCookPolicyCodec
{
	TextureColorSpace ParseColorSpace(std::string_view value);
	TextureMipPolicy ParseMipPolicy(std::string_view value);
	TextureMipFilter ParseMipFilter(std::string_view value);
	TextureColorProcessingPolicy ParseColorProcessingPolicy(std::string_view value);
	TextureGroup ParseTextureGroup(std::string_view value);
	TextureDimension ParseDimension(std::string_view value);
	TextureChannelMask ParseChannelMask(std::string_view value);
}
