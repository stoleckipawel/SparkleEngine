#include "PCH.h"

#include "TextureCookRequestBuilder.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
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
	outRequest.policy.colorSpace = ResolveColorSpace(textureGroup);
	outRequest.policy.mipPolicy = ResolveMipPolicy(textureGroup);
	outRequest.policy.mipFilter = ResolveMipFilter(textureGroup);
	outRequest.policy.colorProcessingPolicy = ResolveColorProcessingPolicy(textureGroup);
	outRequest.policy.textureGroup = textureGroup;
	outRequest.policy.dimension = ResolveTextureDimension(textureGroup);
	outRequest.policy.channelMask = channelMask;

	std::string textureSourceKey;
	if (!BuildTextureSourceKey(outRequest, textureSourceKey, outErrorMessage))
	{
		return false;
	}

	outRequest.assetId = Hash::Fnv1a64(textureSourceKey);
	if (!BuildTextureOutputPath(outRequest, outRequest.outputPath, outErrorMessage))
	{
		return false;
	}
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
	const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, projectRoot))
	{
		outTextureSourceKey = std::string("project:") + GetTextureColorSpaceName(request.policy.colorSpace) + ":" +
		                      GetTextureMipPolicyName(request.policy.mipPolicy) + ":" + GetTextureMipFilterName(request.policy.mipFilter) +
		                      ":" + GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) + ":" +
		                      GetTextureGroupName(request.policy.textureGroup) + ":" + GetTextureDimensionName(request.policy.dimension) +
		                      ":" + GetTextureChannelMaskName(request.policy.channelMask) + ":" + relativePath->generic_string();
		outErrorMessage.clear();
		return true;
	}

	const std::filesystem::path& engineRoot = Filesystem::GetEnginePath();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, engineRoot))
	{
		outTextureSourceKey = std::string("engine:") + GetTextureColorSpaceName(request.policy.colorSpace) + ":" +
		                      GetTextureMipPolicyName(request.policy.mipPolicy) + ":" + GetTextureMipFilterName(request.policy.mipFilter) +
		                      ":" + GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) + ":" +
		                      GetTextureGroupName(request.policy.textureGroup) + ":" + GetTextureDimensionName(request.policy.dimension) +
		                      ":" + GetTextureChannelMaskName(request.policy.channelMask) + ":" + relativePath->generic_string();
		outErrorMessage.clear();
		return true;
	}

	outErrorMessage = "Source texture path must be under the project or engine root to derive a stable cooked texture id: '" +
	                  request.sourcePath.string() + "'";
	return false;
}

bool TextureCookRequestBuilder::BuildTextureOutputPath(
    const TextureCookRequest& request,
    std::filesystem::path& outTextureOutputPath,
    std::string& outErrorMessage)
{
	const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, projectRoot))
	{
		std::filesystem::path cookedRelativePath = std::filesystem::path("Project") / *relativePath;
		cookedRelativePath.replace_extension(BuildTextureVariantSuffix(request) + ".stex");
		outTextureOutputPath = Filesystem::GetCookedTextureRootPath() / cookedRelativePath;
		outErrorMessage.clear();
		return true;
	}

	const std::filesystem::path& engineRoot = Filesystem::GetEnginePath();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, engineRoot))
	{
		std::filesystem::path cookedRelativePath = std::filesystem::path("Engine") / *relativePath;
		cookedRelativePath.replace_extension(BuildTextureVariantSuffix(request) + ".stex");
		outTextureOutputPath = Filesystem::GetCookedTextureRootPath() / cookedRelativePath;
		outErrorMessage.clear();
		return true;
	}

	outErrorMessage = "Source texture path must be under the project or engine root to derive a cooked texture output path: '" +
	                  request.sourcePath.string() + "'";
	return false;
}

std::string TextureCookRequestBuilder::BuildTextureVariantSuffix(const TextureCookRequest& request)
{
	return std::string(".") + GetTextureGroupName(request.policy.textureGroup) + "." + GetTextureColorSpaceName(request.policy.colorSpace) +
	       "." + GetTextureMipPolicyName(request.policy.mipPolicy) + "." + GetTextureMipFilterName(request.policy.mipFilter) + "." +
	       GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) + "." +
	       GetTextureDimensionName(request.policy.dimension) + "." + GetTextureChannelMaskName(request.policy.channelMask);
}



