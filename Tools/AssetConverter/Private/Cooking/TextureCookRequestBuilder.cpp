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
		const TextureColorSpace colorSpace = ResolveColorSpace(semantic);

		std::filesystem::path normalizedSourceTexturePath;
		if (!NormalizeSourceTexturePath(sourceTexturePath, normalizedSourceTexturePath, outErrorMessage))
		{
			return false;
		}

		std::string textureSourceKey;
		if (!BuildTextureSourceKey(normalizedSourceTexturePath, colorSpace, textureSourceKey, outErrorMessage))
		{
			return false;
		}

		outRequest.assetId = Hash::Fnv1a64(textureSourceKey);
		outRequest.sourcePath = normalizedSourceTexturePath;
		outRequest.outputPath = Paths::CookedTextureAsset(outRequest.assetId);
		outRequest.colorSpace = colorSpace;
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
	    const std::filesystem::path& normalizedSourceTexturePath,
	    TextureColorSpace colorSpace,
	    std::string& outTextureSourceKey,
	    std::string& outErrorMessage)
	{
		const std::filesystem::path& projectRoot = Paths::ProjectRoot();
		if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(normalizedSourceTexturePath, projectRoot))
		{
			outTextureSourceKey = std::string(colorSpace == TextureColorSpace::Srgb ? "project:srgb:" : "project:linear:") +
			                     relativePath->generic_string();
			outErrorMessage.clear();
			return true;
		}

		const std::filesystem::path& engineRoot = Paths::EngineRoot();
		if (const auto relativePath = Paths::TryMakeRelativeUnderRoot(normalizedSourceTexturePath, engineRoot))
		{
			outTextureSourceKey = std::string(colorSpace == TextureColorSpace::Srgb ? "engine:srgb:" : "engine:linear:") +
			                     relativePath->generic_string();
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage =
		    "Source texture path must be under the project or engine root to derive a stable cooked texture id: '" +
		    normalizedSourceTexturePath.string() + "'";
		return false;
	}
}