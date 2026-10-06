#include "DefaultTextureCookRequestBuilder.h"

#include "Core/Public/Assets/DefaultTexture.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"

#include <array>
#include <string_view>
#include <system_error>

struct DefaultTextureCookDesc final
{
	std::string_view SourceRelativePath;
	DefaultTexture Product;
	TextureColorSpace ColorSpace;
	TextureMipFilter MipFilter;
	TextureColorProcessingPolicy ColorProcessingPolicy;
	TextureGroup Group;
	TextureDimension Dimension;
};

static constexpr std::array DefaultTextures = {
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_checkerboard.png",
        DefaultTexture::Checkerboard,
        TextureColorSpace::Srgb,
        TextureMipFilter::Kaiser,
        TextureColorProcessingPolicy::SrgbLinearize,
        TextureGroup::Diffuse,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_white.png",
        DefaultTexture::White,
        TextureColorSpace::Srgb,
        TextureMipFilter::Regular,
        TextureColorProcessingPolicy::SrgbLinearize,
        TextureGroup::Diffuse,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_black.png",
        DefaultTexture::Black,
        TextureColorSpace::Srgb,
        TextureMipFilter::Regular,
        TextureColorProcessingPolicy::SrgbLinearize,
        TextureGroup::Diffuse,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_red.png",
        DefaultTexture::Red,
        TextureColorSpace::Srgb,
        TextureMipFilter::Regular,
        TextureColorProcessingPolicy::SrgbLinearize,
        TextureGroup::Diffuse,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_green.png",
        DefaultTexture::Green,
        TextureColorSpace::Srgb,
        TextureMipFilter::Regular,
        TextureColorProcessingPolicy::SrgbLinearize,
        TextureGroup::Diffuse,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_blue.png",
        DefaultTexture::Blue,
        TextureColorSpace::Srgb,
        TextureMipFilter::Regular,
        TextureColorProcessingPolicy::SrgbLinearize,
        TextureGroup::Diffuse,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Defaults/default_normal.png",
        DefaultTexture::Normal,
        TextureColorSpace::Linear,
        TextureMipFilter::NormalAware,
        TextureColorProcessingPolicy::Linear,
        TextureGroup::NormalMap,
        TextureDimension::Texture2D},
    DefaultTextureCookDesc{
        "Assets/Textures/Sky/evening_road_01_puresky_4k.exr",
        DefaultTexture::Sky,
        TextureColorSpace::Linear,
        TextureMipFilter::Regular,
        TextureColorProcessingPolicy::Linear,
        TextureGroup::Default,
        TextureDimension::Texture2D},
};

static void AppendRequest(const DefaultTextureCookDesc& description, TextureCookRequestSet& requestSet)
{
	const std::filesystem::path sourcePath =
	    (Filesystem::GetEnginePath() / std::filesystem::path(description.SourceRelativePath)).lexically_normal();
	std::error_code errorCode;
	if (!std::filesystem::exists(sourcePath, errorCode))
	{
		throw Diagnostics::Error("Default source texture was not found: " + sourcePath.string() + ".");
	}

	const DefaultTextureDesc& product = DefaultTextureDescs[static_cast<std::size_t>(description.Product)];
	TextureCookRequest request;
	request.assetId = Hash::Fnv1a64(std::string("engine-default-texture:") + std::string(product.path));
	request.sourcePath = sourcePath;
	request.outputPath = (Filesystem::GetCookedTextureRootPath() / std::filesystem::path(product.path)).lexically_normal();
	request.policy.colorSpace = description.ColorSpace;
	request.policy.mipPolicy = TextureMipPolicy::Generate;
	request.policy.mipFilter = description.MipFilter;
	request.policy.colorProcessingPolicy = description.ColorProcessingPolicy;
	request.policy.textureGroup = description.Group;
	request.policy.dimension = description.Dimension;
	request.policy.channelMask = TextureChannelMask::Rgba;
	requestSet.Add(request);
}

void DefaultTextureCookRequestBuilder::AppendTo(TextureCookRequestSet& requestSet)
{
	for (const DefaultTextureCookDesc& texture : DefaultTextures)
	{
		AppendRequest(texture, requestSet);
	}
}
