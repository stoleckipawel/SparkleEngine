#include "PCH.h"

#include "Cooking/TextureCookRequestBuilder.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"

	bool TextureCookRequestBuilder::Build(
	    const std::filesystem::path& sourceTexturePath,
	    TextureGroup textureGroup,
	    TextureCookRequest& outRequest,
	    std::string& outErrorMessage,
	    TextureChannelMask channelMask)
	{
		std::filesystem::path normalizedSourceTexturePath;
		if (!NormalizeSourceTexturePath(sourceTexturePath, normalizedSourceTexturePath, outErrorMessage))
		{
			return false;
		}

		outRequest.sourcePath = normalizedSourceTexturePath;
		outRequest.colorSpace = ResolveColorSpace(textureGroup);
		outRequest.mipPolicy = ResolveMipPolicy(textureGroup);
		outRequest.mipFilter = ResolveMipFilter(textureGroup);
		outRequest.colorProcessingPolicy = ResolveColorProcessingPolicy(textureGroup);
		outRequest.textureGroup = textureGroup;
		outRequest.dimension = ResolveTextureDimension(textureGroup);
		outRequest.channelMask = channelMask;

		std::string textureSourceKey;
		if (!BuildTextureSourceKey(outRequest, textureSourceKey, outErrorMessage))
		{
			return false;
		}

		outRequest.assetId = Hash::Fnv1a64(textureSourceKey);
		outRequest.outputPath = Paths::CookedTextureAsset(outRequest.assetId);
		outErrorMessage.clear();
		return true;
	}

	TextureColorSpace TextureCookRequestBuilder::ResolveColorSpace(TextureGroup textureGroup) noexcept
	{
		switch (textureGroup)
		{
			case TextureGroup::Diffuse:
			case TextureGroup::Emissive:
			case TextureGroup::SubsurfaceColor:
				return TextureColorSpace::Srgb;

			case TextureGroup::Default:
			case TextureGroup::NormalMap:
			case TextureGroup::Roughness:
			case TextureGroup::Metallic:
			case TextureGroup::AmbientOcclusion:
			case TextureGroup::SubsurfaceStrength:
			case TextureGroup::HdrColor:
			default:
				return TextureColorSpace::Linear;
		}
	}

	TextureMipPolicy TextureCookRequestBuilder::ResolveMipPolicy(TextureGroup textureGroup) noexcept
	{
		(void) textureGroup;
		return TextureMipPolicy::Generate;
	}

	TextureMipFilter TextureCookRequestBuilder::ResolveMipFilter(TextureGroup textureGroup) noexcept
	{
		switch (textureGroup)
		{
			case TextureGroup::Diffuse:
				return TextureMipFilter::Kaiser;

			case TextureGroup::NormalMap:
				return TextureMipFilter::NormalAware;

			case TextureGroup::Default:
			case TextureGroup::Roughness:
			case TextureGroup::Metallic:
			case TextureGroup::AmbientOcclusion:
			case TextureGroup::Emissive:
			case TextureGroup::SubsurfaceColor:
			case TextureGroup::SubsurfaceStrength:
			case TextureGroup::HdrColor:
			default:
				return TextureMipFilter::Regular;
		}
	}

	TextureColorProcessingPolicy TextureCookRequestBuilder::ResolveColorProcessingPolicy(TextureGroup textureGroup) noexcept
	{
		return ResolveColorSpace(textureGroup) == TextureColorSpace::Srgb ? TextureColorProcessingPolicy::SrgbLinearize
		                                                                  : TextureColorProcessingPolicy::Linear;
	}

	TextureDimension TextureCookRequestBuilder::ResolveTextureDimension(TextureGroup textureGroup) noexcept
	{
		(void) textureGroup;
		return TextureDimension::Texture2D;
	}

	bool TextureCookRequestBuilder::NormalizeSourceTexturePath(
	    const std::filesystem::path& sourceTexturePath,
	    std::filesystem::path& outNormalizedSourceTexturePath,
	    std::string& outErrorMessage)
	{
		if (const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(sourceTexturePath, AssetType::Texture))
		{
			outNormalizedSourceTexturePath = *resolvedPath;
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage = "Unable to resolve source texture path '" + sourceTexturePath.string() + "'";
		return false;
	}

	bool TextureCookRequestBuilder::BuildTextureSourceKey(
	    const TextureCookRequest& request,
	    std::string& outTextureSourceKey,
	    std::string& outErrorMessage)
	{
		const std::filesystem::path& projectRoot = Paths::ProjectRoot();
		if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, projectRoot))
		{
			outTextureSourceKey = std::string("project:") + GetTextureColorSpaceName(request.colorSpace) + ":" +
			                      GetTextureMipPolicyName(request.mipPolicy) + ":" + GetTextureMipFilterName(request.mipFilter) + ":" +
			                      GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) + ":" +
			                      GetTextureGroupName(request.textureGroup) + ":" + GetTextureDimensionName(request.dimension) + ":" +
			                      GetTextureChannelMaskName(request.channelMask) + ":" + relativePath->generic_string();
			outErrorMessage.clear();
			return true;
		}

		const std::filesystem::path& engineRoot = Paths::EngineRoot();
		if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, engineRoot))
		{
			outTextureSourceKey = std::string("engine:") + GetTextureColorSpaceName(request.colorSpace) + ":" +
			                      GetTextureMipPolicyName(request.mipPolicy) + ":" + GetTextureMipFilterName(request.mipFilter) + ":" +
			                      GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) + ":" +
			                      GetTextureGroupName(request.textureGroup) + ":" + GetTextureDimensionName(request.dimension) + ":" +
			                      GetTextureChannelMaskName(request.channelMask) + ":" + relativePath->generic_string();
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage = "Source texture path must be under the project or engine root to derive a stable cooked texture id: '" +
		                  request.sourcePath.string() + "'";
		return false;
	}