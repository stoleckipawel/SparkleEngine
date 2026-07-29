#include "PCH.h"

#include "TextureCookRequestBuilder.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"

std::string TextureCookRequestBuilder::BuildTextureSourceKeyForRoot(
    std::string_view rootName,
    const TextureCookRequest& request,
    const std::filesystem::path& relativePath)
{
	return std::string(rootName) + ":" + GetTextureColorSpaceName(request.policy.colorSpace) + ":" +
	       GetTextureMipPolicyName(request.policy.mipPolicy) + ":" + GetTextureMipFilterName(request.policy.mipFilter) + ":" +
	       GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) + ":" +
	       GetTextureGroupName(request.policy.textureGroup) + ":" + GetTextureDimensionName(request.policy.dimension) + ":" +
	       GetTextureChannelMaskName(request.policy.channelMask) + ":" + relativePath.generic_string();
}

std::optional<std::filesystem::path> TextureCookRequestBuilder::BuildTextureOutputPathForRoot(
    const std::filesystem::path& sourceTexturePath,
    const std::filesystem::path& sourceRoot,
    std::string_view cookedDirectory,
    std::string_view variantSuffix)
{
	const auto relativePath = Paths::TryMakeRelativeUnderRoot(sourceTexturePath, sourceRoot);
	if (!relativePath)
	{
		return std::nullopt;
	}

	std::filesystem::path cookedRelativePath = std::filesystem::path(cookedDirectory) / *relativePath;
	cookedRelativePath.replace_extension(std::string(variantSuffix) + ".stex");
	return Filesystem::GetCookedTextureRootPath() / cookedRelativePath;
}

TextureCookRequest TextureCookRequestBuilder::Build(
    const std::filesystem::path& sourceTexturePath,
    TextureGroup textureGroup,
    TextureChannelMask channelMask)
{
	TextureCookRequest request;
	request.sourcePath = NormalizeSourceTexturePath(sourceTexturePath);
	request.policy.colorSpace = ResolveColorSpace(textureGroup);
	request.policy.mipPolicy = ResolveMipPolicy(textureGroup);
	request.policy.mipFilter = ResolveMipFilter(textureGroup);
	request.policy.colorProcessingPolicy = ResolveColorProcessingPolicy(textureGroup);
	request.policy.textureGroup = textureGroup;
	request.policy.dimension = ResolveTextureDimension(textureGroup);
	request.policy.channelMask = channelMask;
	request.assetId = Hash::Fnv1a64(BuildTextureSourceKey(request));
	request.outputPath = BuildTextureOutputPath(request);
	return request;
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

std::filesystem::path TextureCookRequestBuilder::NormalizeSourceTexturePath(const std::filesystem::path& sourceTexturePath)
{
	if (const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(sourceTexturePath, AssetType::Texture))
	{
		return *resolvedPath;
	}

	throw Diagnostics::Error("Unable to resolve source texture path '" + sourceTexturePath.string() + "'.");
}

std::string TextureCookRequestBuilder::BuildTextureSourceKey(const TextureCookRequest& request)
{
	const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, projectRoot))
	{
		return BuildTextureSourceKeyForRoot("project", request, *relativePath);
	}

	const std::filesystem::path& engineRoot = Filesystem::GetEnginePath();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, engineRoot))
	{
		return BuildTextureSourceKeyForRoot("engine", request, *relativePath);
	}

	const std::filesystem::path importedTextureRoot = Paths::ImportedTextureCacheRoot();
	if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, importedTextureRoot))
	{
		return BuildTextureSourceKeyForRoot("imported", request, *relativePath);
	}

	throw Diagnostics::Error(
	    "Source texture path must be under the project, engine, or imported-texture cache root "
	    "to derive a stable cooked texture id: '" +
	    request.sourcePath.string() + "'.");
}

std::filesystem::path TextureCookRequestBuilder::BuildTextureOutputPath(const TextureCookRequest& request)
{
	const std::string variantSuffix = BuildTextureVariantSuffix(request);
	const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
	if (const auto outputPath = BuildTextureOutputPathForRoot(request.sourcePath, projectRoot, "Project", variantSuffix))
	{
		return *outputPath;
	}

	const std::filesystem::path& engineRoot = Filesystem::GetEnginePath();
	if (const auto outputPath = BuildTextureOutputPathForRoot(request.sourcePath, engineRoot, "Engine", variantSuffix))
	{
		return *outputPath;
	}

	if (const auto outputPath =
	        BuildTextureOutputPathForRoot(request.sourcePath, Paths::ImportedTextureCacheRoot(), "Imported", variantSuffix))
	{
		return *outputPath;
	}

	throw Diagnostics::Error(
	    "Source texture path must be under the project, engine, or imported-texture cache root "
	    "to derive a cooked texture output path: '" +
	    request.sourcePath.string() + "'.");
}

std::string TextureCookRequestBuilder::BuildTextureVariantSuffix(const TextureCookRequest& request)
{
	return std::string(".") + GetTextureGroupName(request.policy.textureGroup) + "." + GetTextureColorSpaceName(request.policy.colorSpace) +
	       "." + GetTextureMipPolicyName(request.policy.mipPolicy) + "." + GetTextureMipFilterName(request.policy.mipFilter) + "." +
	       GetTextureColorProcessingPolicyName(request.policy.colorProcessingPolicy) + "." +
	       GetTextureDimensionName(request.policy.dimension) + "." + GetTextureChannelMaskName(request.policy.channelMask);
}
