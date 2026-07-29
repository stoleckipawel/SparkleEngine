#include "TextureCookPolicyCodec.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Strings/StringUtils.h"

static const auto g_textureCookPolicyCodecLogger = Logging::GetOrCreateLogger("TextureCooker.PolicyCodec");

TextureColorSpace TextureCookPolicyCodec::ParseColorSpace(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "linear"))
	{
		return TextureColorSpace::Linear;
	}
	if (Strings::EqualsIgnoreCase(value, "srgb"))
	{
		return TextureColorSpace::Srgb;
	}
	throw Diagnostics::Error("Texture cook request entry has an unknown color space '" + std::string(value) + "'.");
}

TextureMipPolicy TextureCookPolicyCodec::ParseMipPolicy(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "generate"))
	{
		return TextureMipPolicy::Generate;
	}
	if (Strings::EqualsIgnoreCase(value, "preserve-existing") || Strings::EqualsIgnoreCase(value, "preserveexisting"))
	{
		return TextureMipPolicy::PreserveExisting;
	}
	if (Strings::EqualsIgnoreCase(value, "no-mips") || Strings::EqualsIgnoreCase(value, "nomips"))
	{
		return TextureMipPolicy::NoMips;
	}
	throw Diagnostics::Error("Texture cook request entry has an unknown mip policy '" + std::string(value) + "'.");
}

TextureMipFilter TextureCookPolicyCodec::ParseMipFilter(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "regular"))
	{
		return TextureMipFilter::Regular;
	}
	if (Strings::EqualsIgnoreCase(value, "kaiser"))
	{
		return TextureMipFilter::Kaiser;
	}
	if (Strings::EqualsIgnoreCase(value, "normal-aware") || Strings::EqualsIgnoreCase(value, "normalaware"))
	{
		return TextureMipFilter::NormalAware;
	}
	if (Strings::EqualsIgnoreCase(value, "angular"))
	{
		return TextureMipFilter::Angular;
	}
	throw Diagnostics::Error("Texture cook request entry has an unknown mip filter '" + std::string(value) + "'.");
}

TextureColorProcessingPolicy TextureCookPolicyCodec::ParseColorProcessingPolicy(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "linear"))
	{
		return TextureColorProcessingPolicy::Linear;
	}
	if (Strings::EqualsIgnoreCase(value, "srgb-linearize") || Strings::EqualsIgnoreCase(value, "srgblinearize"))
	{
		return TextureColorProcessingPolicy::SrgbLinearize;
	}
	throw Diagnostics::Error(
	    "Texture cook request entry has an unknown color processing policy '" + std::string(value) + "'.");
}

TextureGroup TextureCookPolicyCodec::ParseTextureGroup(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "default") || Strings::EqualsIgnoreCase(value, "none"))
	{
		return TextureGroup::Default;
	}
	if (Strings::EqualsIgnoreCase(value, "diffuse") || Strings::EqualsIgnoreCase(value, "albedo") ||
	    Strings::EqualsIgnoreCase(value, "base-color") || Strings::EqualsIgnoreCase(value, "basecolor") ||
	    Strings::EqualsIgnoreCase(value, "color"))
	{
		return TextureGroup::Diffuse;
	}
	if (Strings::EqualsIgnoreCase(value, "roughness"))
	{
		return TextureGroup::Roughness;
	}
	if (Strings::EqualsIgnoreCase(value, "metallic") || Strings::EqualsIgnoreCase(value, "metalness"))
	{
		return TextureGroup::Metallic;
	}
	if (Strings::EqualsIgnoreCase(value, "ambient-occlusion") ||
	    Strings::EqualsIgnoreCase(value, "ambientocclusion") || Strings::EqualsIgnoreCase(value, "occlusion") ||
	    Strings::EqualsIgnoreCase(value, "ao") || Strings::EqualsIgnoreCase(value, "masks"))
	{
		return TextureGroup::AmbientOcclusion;
	}
	if (Strings::EqualsIgnoreCase(value, "normal-map") || Strings::EqualsIgnoreCase(value, "normalmap"))
	{
		return TextureGroup::NormalMap;
	}
	if (Strings::EqualsIgnoreCase(value, "hdr-color") || Strings::EqualsIgnoreCase(value, "hdrcolor"))
	{
		return TextureGroup::HdrColor;
	}
	if (Strings::EqualsIgnoreCase(value, "emissive") || Strings::EqualsIgnoreCase(value, "emission"))
	{
		return TextureGroup::Emissive;
	}
	if (Strings::EqualsIgnoreCase(value, "subsurface-color") ||
	    Strings::EqualsIgnoreCase(value, "subsurfacecolor"))
	{
		return TextureGroup::SubsurfaceColor;
	}
	if (Strings::EqualsIgnoreCase(value, "subsurface-strength") ||
	    Strings::EqualsIgnoreCase(value, "subsurfacestrength"))
	{
		return TextureGroup::SubsurfaceStrength;
	}
	throw Diagnostics::Error("Texture cook request entry has an unknown texture group '" + std::string(value) + "'.");
}

TextureDimension TextureCookPolicyCodec::ParseDimension(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "2d") || Strings::EqualsIgnoreCase(value, "texture2d"))
	{
		return TextureDimension::Texture2D;
	}
	if (Strings::EqualsIgnoreCase(value, "cube") || Strings::EqualsIgnoreCase(value, "texturecube"))
	{
		return TextureDimension::TextureCube;
	}
	throw Diagnostics::Error("Texture cook request entry has an unknown texture dimension '" + std::string(value) + "'.");
}

TextureChannelMask TextureCookPolicyCodec::ParseChannelMask(std::string_view value)
{
	if (Strings::EqualsIgnoreCase(value, "rgba") || Strings::EqualsIgnoreCase(value, "all") ||
	    Strings::EqualsIgnoreCase(value, "none"))
	{
		return TextureChannelMask::Rgba;
	}
	if (Strings::EqualsIgnoreCase(value, "r") || Strings::EqualsIgnoreCase(value, "red"))
	{
		return TextureChannelMask::Red;
	}
	if (Strings::EqualsIgnoreCase(value, "g") || Strings::EqualsIgnoreCase(value, "green"))
	{
		return TextureChannelMask::Green;
	}
	if (Strings::EqualsIgnoreCase(value, "b") || Strings::EqualsIgnoreCase(value, "blue"))
	{
		return TextureChannelMask::Blue;
	}
	if (Strings::EqualsIgnoreCase(value, "a") || Strings::EqualsIgnoreCase(value, "alpha"))
	{
		return TextureChannelMask::Alpha;
	}
	throw Diagnostics::Error("Texture cook request entry has an unknown channel mask '" + std::string(value) + "'.");
}

const char* GetTextureColorSpaceName(TextureColorSpace colorSpace) noexcept
{
	switch (colorSpace)
	{
		case TextureColorSpace::Linear: return "linear";
		case TextureColorSpace::Srgb: return "srgb";
	}
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture color space.");
}

const char* GetTextureMipPolicyName(TextureMipPolicy mipPolicy) noexcept
{
	switch (mipPolicy)
	{
		case TextureMipPolicy::Generate: return "generate";
		case TextureMipPolicy::PreserveExisting: return "preserve-existing";
		case TextureMipPolicy::NoMips: return "no-mips";
	}
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture mip policy.");
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
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture mip filter.");
}

const char* GetTextureColorProcessingPolicyName(TextureColorProcessingPolicy policy) noexcept
{
	switch (policy)
	{
		case TextureColorProcessingPolicy::Linear: return "linear";
		case TextureColorProcessingPolicy::SrgbLinearize: return "srgb-linearize";
	}
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture color processing policy.");
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
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture group.");
}

const char* GetTextureDimensionName(TextureDimension dimension) noexcept
{
	switch (dimension)
	{
		case TextureDimension::Texture2D: return "2d";
		case TextureDimension::TextureCube: return "cube";
	}
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture dimension.");
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
	Diagnostics::Fatal(g_textureCookPolicyCodecLogger, __FILE__, __LINE__, "Unknown texture channel mask.");
}
