#include "TextureCookPolicyCodec.h"

#include "Core/Public/Strings/StringUtils.h"

bool TextureCookPolicyCodec::TryParseColorSpace(std::string_view value, TextureColorSpace& outColorSpace) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "linear"))
	{
		outColorSpace = TextureColorSpace::Linear;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "srgb"))
	{
		outColorSpace = TextureColorSpace::Srgb;
		return true;
	}
	return false;
}

bool TextureCookPolicyCodec::TryParseMipPolicy(std::string_view value, TextureMipPolicy& outMipPolicy) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "generate"))
	{
		outMipPolicy = TextureMipPolicy::Generate;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "preserve-existing") || Strings::EqualsIgnoreCase(value, "preserveexisting"))
	{
		outMipPolicy = TextureMipPolicy::PreserveExisting;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "no-mips") || Strings::EqualsIgnoreCase(value, "nomips"))
	{
		outMipPolicy = TextureMipPolicy::NoMips;
		return true;
	}
	return false;
}

bool TextureCookPolicyCodec::TryParseMipFilter(std::string_view value, TextureMipFilter& outMipFilter) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "regular"))
	{
		outMipFilter = TextureMipFilter::Regular;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "kaiser"))
	{
		outMipFilter = TextureMipFilter::Kaiser;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "normal-aware") || Strings::EqualsIgnoreCase(value, "normalaware"))
	{
		outMipFilter = TextureMipFilter::NormalAware;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "angular"))
	{
		outMipFilter = TextureMipFilter::Angular;
		return true;
	}
	return false;
}

bool TextureCookPolicyCodec::TryParseColorProcessingPolicy(
    std::string_view value, TextureColorProcessingPolicy& outColorProcessingPolicy) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "linear"))
	{
		outColorProcessingPolicy = TextureColorProcessingPolicy::Linear;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "srgb-linearize") || Strings::EqualsIgnoreCase(value, "srgblinearize"))
	{
		outColorProcessingPolicy = TextureColorProcessingPolicy::SrgbLinearize;
		return true;
	}
	return false;
}

bool TextureCookPolicyCodec::TryParseTextureGroup(std::string_view value, TextureGroup& outTextureGroup) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "default") || Strings::EqualsIgnoreCase(value, "none"))
	{
		outTextureGroup = TextureGroup::Default;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "diffuse") || Strings::EqualsIgnoreCase(value, "albedo") ||
	    Strings::EqualsIgnoreCase(value, "base-color") || Strings::EqualsIgnoreCase(value, "basecolor") ||
	    Strings::EqualsIgnoreCase(value, "color"))
	{
		outTextureGroup = TextureGroup::Diffuse;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "roughness"))
	{
		outTextureGroup = TextureGroup::Roughness;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "metallic") || Strings::EqualsIgnoreCase(value, "metalness"))
	{
		outTextureGroup = TextureGroup::Metallic;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "ambient-occlusion") ||
	    Strings::EqualsIgnoreCase(value, "ambientocclusion") || Strings::EqualsIgnoreCase(value, "occlusion") ||
	    Strings::EqualsIgnoreCase(value, "ao") || Strings::EqualsIgnoreCase(value, "masks"))
	{
		outTextureGroup = TextureGroup::AmbientOcclusion;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "normal-map") || Strings::EqualsIgnoreCase(value, "normalmap"))
	{
		outTextureGroup = TextureGroup::NormalMap;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "hdr-color") || Strings::EqualsIgnoreCase(value, "hdrcolor"))
	{
		outTextureGroup = TextureGroup::HdrColor;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "emissive") || Strings::EqualsIgnoreCase(value, "emission"))
	{
		outTextureGroup = TextureGroup::Emissive;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "subsurface-color") ||
	    Strings::EqualsIgnoreCase(value, "subsurfacecolor"))
	{
		outTextureGroup = TextureGroup::SubsurfaceColor;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "subsurface-strength") ||
	    Strings::EqualsIgnoreCase(value, "subsurfacestrength"))
	{
		outTextureGroup = TextureGroup::SubsurfaceStrength;
		return true;
	}
	return false;
}

bool TextureCookPolicyCodec::TryParseDimension(std::string_view value, TextureDimension& outDimension) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "2d") || Strings::EqualsIgnoreCase(value, "texture2d"))
	{
		outDimension = TextureDimension::Texture2D;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "cube") || Strings::EqualsIgnoreCase(value, "texturecube"))
	{
		outDimension = TextureDimension::TextureCube;
		return true;
	}
	return false;
}

bool TextureCookPolicyCodec::TryParseChannelMask(std::string_view value, TextureChannelMask& outChannelMask) noexcept
{
	if (Strings::EqualsIgnoreCase(value, "rgba") || Strings::EqualsIgnoreCase(value, "all") ||
	    Strings::EqualsIgnoreCase(value, "none"))
	{
		outChannelMask = TextureChannelMask::Rgba;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "r") || Strings::EqualsIgnoreCase(value, "red"))
	{
		outChannelMask = TextureChannelMask::Red;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "g") || Strings::EqualsIgnoreCase(value, "green"))
	{
		outChannelMask = TextureChannelMask::Green;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "b") || Strings::EqualsIgnoreCase(value, "blue"))
	{
		outChannelMask = TextureChannelMask::Blue;
		return true;
	}
	if (Strings::EqualsIgnoreCase(value, "a") || Strings::EqualsIgnoreCase(value, "alpha"))
	{
		outChannelMask = TextureChannelMask::Alpha;
		return true;
	}
	return false;
}

const char* GetTextureColorSpaceName(TextureColorSpace colorSpace) noexcept
{
	switch (colorSpace)
	{
		case TextureColorSpace::Linear: return "linear";
		case TextureColorSpace::Srgb: return "srgb";
	}
	return "linear";
}

const char* GetTextureMipPolicyName(TextureMipPolicy mipPolicy) noexcept
{
	switch (mipPolicy)
	{
		case TextureMipPolicy::Generate: return "generate";
		case TextureMipPolicy::PreserveExisting: return "preserve-existing";
		case TextureMipPolicy::NoMips: return "no-mips";
	}
	return "generate";
}

const char* GetTextureMipFilterName(TextureMipFilter mipFilter) noexcept
{
	switch (mipFilter)
	{
		case TextureMipFilter::Regular: return "regular";
		case TextureMipFilter::Kaiser: return "kaiser";
		case TextureMipFilter::NormalAware: return "normal-aware";
		case TextureMipFilter::Angular: return "angular";
	}
	return "regular";
}

const char* GetTextureColorProcessingPolicyName(TextureColorProcessingPolicy policy) noexcept
{
	switch (policy)
	{
		case TextureColorProcessingPolicy::Linear: return "linear";
		case TextureColorProcessingPolicy::SrgbLinearize: return "srgb-linearize";
	}
	return "linear";
}

const char* GetTextureGroupName(TextureGroup textureGroup) noexcept
{
	switch (textureGroup)
	{
		case TextureGroup::Default: return "default";
		case TextureGroup::Diffuse: return "diffuse";
		case TextureGroup::NormalMap: return "normal-map";
		case TextureGroup::Roughness: return "roughness";
		case TextureGroup::Metallic: return "metallic";
		case TextureGroup::AmbientOcclusion: return "ambient-occlusion";
		case TextureGroup::Emissive: return "emissive";
		case TextureGroup::SubsurfaceColor: return "subsurface-color";
		case TextureGroup::SubsurfaceStrength: return "subsurface-strength";
		case TextureGroup::HdrColor: return "hdr-color";
	}
	return "default";
}

const char* GetTextureDimensionName(TextureDimension dimension) noexcept
{
	switch (dimension)
	{
		case TextureDimension::Texture2D: return "2d";
		case TextureDimension::TextureCube: return "cube";
	}
	return "2d";
}

const char* GetTextureChannelMaskName(TextureChannelMask channelMask) noexcept
{
	switch (channelMask)
	{
		case TextureChannelMask::Rgba: return "rgba";
		case TextureChannelMask::Red: return "red";
		case TextureChannelMask::Green: return "green";
		case TextureChannelMask::Blue: return "blue";
		case TextureChannelMask::Alpha: return "alpha";
	}
	return "rgba";
}
