#include "DefaultTextureCookRequestBuilder.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "ToolConsole.h"

#include <array>
#include <iostream>
#include <string_view>
#include <system_error>

namespace
{
	struct DefaultTextureCookDesc final
	{
		std::string_view SourceRelativePath;
		std::string_view OutputRelativePath;
		TextureColorSpace ColorSpace;
		TextureMipFilter MipFilter;
		TextureColorProcessingPolicy ColorProcessingPolicy;
		TextureGroup Group;
		TextureDimension Dimension;
	};

	constexpr std::array DefaultTextures = {
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_checkerboard.png",
	        "Defaults/default_checkerboard.stex",
	        TextureColorSpace::Srgb,
	        TextureMipFilter::Kaiser,
	        TextureColorProcessingPolicy::SrgbLinearize,
	        TextureGroup::Diffuse,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_white.png",
	        "Defaults/default_white.stex",
	        TextureColorSpace::Srgb,
	        TextureMipFilter::Regular,
	        TextureColorProcessingPolicy::SrgbLinearize,
	        TextureGroup::Diffuse,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_black.png",
	        "Defaults/default_black.stex",
	        TextureColorSpace::Srgb,
	        TextureMipFilter::Regular,
	        TextureColorProcessingPolicy::SrgbLinearize,
	        TextureGroup::Diffuse,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_red.png",
	        "Defaults/default_red.stex",
	        TextureColorSpace::Srgb,
	        TextureMipFilter::Regular,
	        TextureColorProcessingPolicy::SrgbLinearize,
	        TextureGroup::Diffuse,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_green.png",
	        "Defaults/default_green.stex",
	        TextureColorSpace::Srgb,
	        TextureMipFilter::Regular,
	        TextureColorProcessingPolicy::SrgbLinearize,
	        TextureGroup::Diffuse,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_blue.png",
	        "Defaults/default_blue.stex",
	        TextureColorSpace::Srgb,
	        TextureMipFilter::Regular,
	        TextureColorProcessingPolicy::SrgbLinearize,
	        TextureGroup::Diffuse,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Defaults/default_normal.png",
	        "Defaults/default_normal.stex",
	        TextureColorSpace::Linear,
	        TextureMipFilter::NormalAware,
	        TextureColorProcessingPolicy::Linear,
	        TextureGroup::NormalMap,
	        TextureDimension::Texture2D},
	    DefaultTextureCookDesc{
	        "Assets/Textures/Sky/evening_road_01_puresky_4k.exr",
	        "Defaults/default_cubemap.stex",
	        TextureColorSpace::Linear,
	        TextureMipFilter::Regular,
	        TextureColorProcessingPolicy::Linear,
	        TextureGroup::Default,
	        TextureDimension::Texture2D},
	};

	bool AppendRequest(
	    const DefaultTextureCookDesc& description,
	    TextureCookRequestSet& requestSet,
	    std::string& outErrorMessage)
	{
		const std::filesystem::path sourcePath =
		    (Filesystem::GetEnginePath() / std::filesystem::path(description.SourceRelativePath)).lexically_normal();
		std::error_code errorCode;
		if (!std::filesystem::exists(sourcePath, errorCode))
		{
			outErrorMessage = "Default source texture was not found: " + sourcePath.string();
			return false;
		}

		TextureCookRequest request;
		request.assetId = Hash::Fnv1a64(
		    std::string("engine-default-texture:") + std::string(description.OutputRelativePath));
		request.sourcePath = sourcePath;
		request.outputPath =
		    (Filesystem::GetCookedTextureRootPath() / std::filesystem::path(description.OutputRelativePath)).lexically_normal();
		request.policy.colorSpace = description.ColorSpace;
		request.policy.mipPolicy = TextureMipPolicy::Generate;
		request.policy.mipFilter = description.MipFilter;
		request.policy.colorProcessingPolicy = description.ColorProcessingPolicy;
		request.policy.textureGroup = description.Group;
		request.policy.dimension = description.Dimension;
		request.policy.channelMask = TextureChannelMask::Rgba;
		if (!requestSet.Add(request, outErrorMessage))
		{
			return false;
		}

		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Queued default texture",
		    {ToolConsole::QuotedField("name", ToolConsole::PathDisplayName(request.sourcePath)),
		     ToolConsole::PathField("output", request.outputPath)});
		outErrorMessage.clear();
		return true;
	}
}

bool DefaultTextureCookRequestBuilder::AppendTo(
    TextureCookRequestSet& requestSet, std::string& outErrorMessage)
{
	for (const DefaultTextureCookDesc& texture : DefaultTextures)
	{
		if (!AppendRequest(texture, requestSet, outErrorMessage))
		{
			return false;
		}
	}
	outErrorMessage.clear();
	return true;
}
