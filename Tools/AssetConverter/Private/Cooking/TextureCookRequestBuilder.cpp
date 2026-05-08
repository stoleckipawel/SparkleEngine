#include "PCH.h"

#include "Cooking/TextureCookRequestBuilder.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"

namespace AssetAuthoring
{
	bool TextureCookRequestBuilder::Build(
	    const std::filesystem::path& sourceTexturePath,
	    Assets::CookedTextureSemantic semantic,
	    TextureCookRequest& outRequest,
	    std::string& outErrorMessage)
	{
		std::filesystem::path normalizedSourceTexturePath;
		if (!NormalizeSourceTexturePath(sourceTexturePath, normalizedSourceTexturePath, outErrorMessage))
		{
			return false;
		}

		outRequest.sourcePath = normalizedSourceTexturePath;
		outRequest.colorSpace = ResolveColorSpace(semantic);
		outRequest.mipPolicy = ResolveMipPolicy(semantic);
		outRequest.mipFilter = ResolveMipFilter(semantic);
		outRequest.colorProcessingPolicy = ResolveColorProcessingPolicy(semantic);
		outRequest.compressionFamilyPreference = ResolveCompressionFamilyPreference(semantic);
		outRequest.dimension = ResolveTextureDimension(semantic);

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

	TextureColorSpace TextureCookRequestBuilder::ResolveColorSpace(Assets::CookedTextureSemantic semantic) noexcept
	{
		switch (semantic)
		{
			case Assets::CookedTextureSemantic::Albedo:
			case Assets::CookedTextureSemantic::Emissive:
				return TextureColorSpace::Srgb;

			case Assets::CookedTextureSemantic::Normal:
			case Assets::CookedTextureSemantic::MetallicRoughness:
			case Assets::CookedTextureSemantic::Occlusion:
			default:
				return TextureColorSpace::Linear;
		}
	}

	TextureMipPolicy TextureCookRequestBuilder::ResolveMipPolicy(Assets::CookedTextureSemantic semantic) noexcept
	{
		(void)semantic;
		return TextureMipPolicy::Generate;
	}

	TextureMipFilter TextureCookRequestBuilder::ResolveMipFilter(Assets::CookedTextureSemantic semantic) noexcept
	{
		switch (semantic)
		{
			case Assets::CookedTextureSemantic::Albedo:
				return TextureMipFilter::Kaiser;

			case Assets::CookedTextureSemantic::Normal:
				return TextureMipFilter::NormalAware;

			case Assets::CookedTextureSemantic::MetallicRoughness:
			case Assets::CookedTextureSemantic::Occlusion:
			case Assets::CookedTextureSemantic::Emissive:
			default:
				return TextureMipFilter::Regular;
		}
	}

	TextureColorProcessingPolicy TextureCookRequestBuilder::ResolveColorProcessingPolicy(
		Assets::CookedTextureSemantic semantic) noexcept
	{
		return ResolveColorSpace(semantic) == TextureColorSpace::Srgb ? TextureColorProcessingPolicy::SrgbLinearize
		                                                           : TextureColorProcessingPolicy::Linear;
	}

	TextureCompressionFamilyPreference TextureCookRequestBuilder::ResolveCompressionFamilyPreference(
		Assets::CookedTextureSemantic semantic) noexcept
	{
		switch (semantic)
		{
			case Assets::CookedTextureSemantic::Albedo:
			case Assets::CookedTextureSemantic::Emissive:
				return TextureCompressionFamilyPreference::Color;

			case Assets::CookedTextureSemantic::Normal:
				return TextureCompressionFamilyPreference::NormalMap;

			case Assets::CookedTextureSemantic::MetallicRoughness:
			case Assets::CookedTextureSemantic::Occlusion:
			default:
				return TextureCompressionFamilyPreference::Masks;
		}
	}

	TextureDimension TextureCookRequestBuilder::ResolveTextureDimension(Assets::CookedTextureSemantic semantic) noexcept
	{
		(void)semantic;
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
			                     GetTextureMipPolicyName(request.mipPolicy) + ":" +
			                     GetTextureMipFilterName(request.mipFilter) + ":" +
			                     GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) + ":" +
			                     GetTextureCompressionFamilyPreferenceName(request.compressionFamilyPreference) + ":" +
			                     GetTextureDimensionName(request.dimension) + ":" +
			                     relativePath->generic_string();
			outErrorMessage.clear();
			return true;
		}

		const std::filesystem::path& engineRoot = Paths::EngineRoot();
		if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(request.sourcePath, engineRoot))
		{
			outTextureSourceKey = std::string("engine:") + GetTextureColorSpaceName(request.colorSpace) + ":" +
			                     GetTextureMipPolicyName(request.mipPolicy) + ":" +
			                     GetTextureMipFilterName(request.mipFilter) + ":" +
			                     GetTextureColorProcessingPolicyName(request.colorProcessingPolicy) + ":" +
			                     GetTextureCompressionFamilyPreferenceName(request.compressionFamilyPreference) + ":" +
			                     GetTextureDimensionName(request.dimension) + ":" +
			                     relativePath->generic_string();
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage =
		    "Source texture path must be under the project or engine root to derive a stable cooked texture id: '" +
		    request.sourcePath.string() + "'";
		return false;
	}
}