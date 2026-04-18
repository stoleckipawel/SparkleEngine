#include "PCH.h"

#include "Cooking/TextureCookRequestBuilder.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "GameFramework/Public/Assets/Cooked/CookedTextureAssetUtils.h"

#include <system_error>

namespace Engine::AssetAuthoring
{
	bool TextureCookRequestBuilder::Build(
	    const std::filesystem::path& sourceTexturePath,
	    Engine::Assets::CookedTextureSemantic semantic,
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
		outRequest.outputPath = Engine::Assets::BuildCookedTextureAssetPath(
		    static_cast<Engine::Assets::CookedAssetId>(outRequest.assetId));
		outRequest.colorSpace = colorSpace;
		outErrorMessage.clear();
		return true;
	}

	TextureColorSpace TextureCookRequestBuilder::ResolveColorSpace(Engine::Assets::CookedTextureSemantic semantic) noexcept
	{
		switch (semantic)
		{
			case Engine::Assets::CookedTextureSemantic::Albedo:
			case Engine::Assets::CookedTextureSemantic::Emissive:
				return TextureColorSpace::Srgb;

			case Engine::Assets::CookedTextureSemantic::Normal:
			case Engine::Assets::CookedTextureSemantic::MetallicRoughness:
			case Engine::Assets::CookedTextureSemantic::Occlusion:
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

		const std::filesystem::path normalizedPath = Engine::Paths::Normalize(sourceTexturePath);
		if (!normalizedPath.empty() && normalizedPath.is_absolute())
		{
			std::error_code errorCode;
			if (std::filesystem::exists(normalizedPath, errorCode) && !errorCode)
			{
				outNormalizedSourceTexturePath = normalizedPath;
				outErrorMessage.clear();
				return true;
			}
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
		std::error_code errorCode;

		const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
		if (!projectRoot.empty())
		{
			const std::filesystem::path relativePath = std::filesystem::relative(normalizedSourceTexturePath, projectRoot, errorCode);
			const std::string relativePathString = relativePath.generic_string();
			if (!errorCode && !relativePathString.empty() && !relativePathString.starts_with(".."))
			{
				outTextureSourceKey = std::string(colorSpace == TextureColorSpace::Srgb ? "project:srgb:" : "project:linear:") +
				                     relativePathString;
				outErrorMessage.clear();
				return true;
			}
		}

		errorCode.clear();
		const std::filesystem::path& engineRoot = Filesystem::GetEnginePath();
		if (!engineRoot.empty())
		{
			const std::filesystem::path relativePath = std::filesystem::relative(normalizedSourceTexturePath, engineRoot, errorCode);
			const std::string relativePathString = relativePath.generic_string();
			if (!errorCode && !relativePathString.empty() && !relativePathString.starts_with(".."))
			{
				outTextureSourceKey = std::string(colorSpace == TextureColorSpace::Srgb ? "engine:srgb:" : "engine:linear:") +
				                     relativePathString;
				outErrorMessage.clear();
				return true;
			}
		}

		outErrorMessage =
		    "Source texture path must be under the project or engine root to derive a stable cooked texture id: '" +
		    normalizedSourceTexturePath.string() + "'";
		return false;
	}
}