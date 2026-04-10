#include "PCH.h"

#include "Cooking/KtxTextureCooker.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "D3D12/Textures/TextureLoader.h"

#include <ktx.h>

#include <format>
#include <system_error>

namespace Engine::AssetAuthoring
{
	namespace
	{
		inline constexpr ktx_uint32_t kVkFormatR8G8B8A8Unorm = 37;
		inline constexpr ktx_uint32_t kVkFormatR8G8B8A8Srgb = 43;
		inline constexpr ktx_uint32_t kVkFormatB8G8R8A8Unorm = 44;
		inline constexpr ktx_uint32_t kVkFormatB8G8R8A8Srgb = 50;
		inline constexpr ktx_uint32_t kVkFormatBc1RgbaUnorm = 133;
		inline constexpr ktx_uint32_t kVkFormatBc1RgbaSrgb = 134;
		inline constexpr ktx_uint32_t kVkFormatBc2Unorm = 135;
		inline constexpr ktx_uint32_t kVkFormatBc2Srgb = 136;
		inline constexpr ktx_uint32_t kVkFormatBc3Unorm = 137;
		inline constexpr ktx_uint32_t kVkFormatBc3Srgb = 138;
		inline constexpr ktx_uint32_t kVkFormatBc4Unorm = 139;
		inline constexpr ktx_uint32_t kVkFormatBc4Snorm = 140;
		inline constexpr ktx_uint32_t kVkFormatBc5Unorm = 141;
		inline constexpr ktx_uint32_t kVkFormatBc5Snorm = 142;

		struct KtxTextureHandle final
		{
			ktxTexture2* texture = nullptr;

			~KtxTextureHandle()
			{
				if (texture != nullptr)
				{
					ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(texture));
				}
			}
		};
	}

	bool KtxTextureCooker::BuildTextureAsset(
	    const std::filesystem::path& sourceTexturePath,
	    Engine::Assets::CookedTextureSemantic semantic,
	    CookedTextureAssetBuild& outTextureAsset,
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

		outTextureAsset.assetId = Hash::Fnv1a64(textureSourceKey);
		outTextureAsset.sourcePath = normalizedSourceTexturePath;
		outTextureAsset.isSrgb = colorSpace == TextureColorSpace::Srgb;
		outErrorMessage.clear();
		return true;
	}

	std::filesystem::path KtxTextureCooker::BuildTextureAssetPath(Engine::Assets::CookedAssetId textureAssetId)
	{
		return Filesystem::GetProjectAssetsPath() / "Cooked" / "Textures" / std::format("{:016X}.ktx2", textureAssetId);
	}

	bool KtxTextureCooker::Cook(const CookedTextureAssetBuild& textureAsset, std::string& outErrorMessage) const
	{
		if (textureAsset.assetId == Engine::Assets::InvalidCookedAssetId)
		{
			outErrorMessage = "Cooked texture asset id is invalid";
			return false;
		}

		const TextureLoadResult loadResult = TextureLoader::Load(textureAsset.sourcePath);
		if (!loadResult.IsValid())
		{
			outErrorMessage = "Failed to load source texture '" + textureAsset.sourcePath.string() + "'";
			return false;
		}

		ktx_uint32_t vkFormat = 0;
		if (!ResolveVkFormat(
		        loadResult.dxgiFormat,
		        textureAsset.isSrgb ? TextureColorSpace::Srgb : TextureColorSpace::Linear,
		        vkFormat,
		        outErrorMessage))
		{
			return false;
		}

		ktxTextureCreateInfo createInfo{};
		createInfo.glInternalformat = 0;
		createInfo.vkFormat = vkFormat;
		createInfo.baseWidth = loadResult.width;
		createInfo.baseHeight = loadResult.height;
		createInfo.baseDepth = 1;
		createInfo.numDimensions = 2;
		createInfo.numLevels = loadResult.GetMipCount();
		createInfo.numLayers = 1;
		createInfo.numFaces = 1;
		createInfo.isArray = KTX_FALSE;
		createInfo.generateMipmaps = KTX_FALSE;

		KtxTextureHandle textureHandle;
		const KTX_error_code createResult =
		    ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureHandle.texture);
		if (createResult != KTX_SUCCESS || textureHandle.texture == nullptr)
		{
			outErrorMessage = std::format(
			    "Failed to create KTX2 texture container for '{}' (error {})",
			    textureAsset.sourcePath.string(),
			    static_cast<int>(createResult));
			return false;
		}

		for (ktx_uint32_t mipIndex = 0; mipIndex < loadResult.GetMipCount(); ++mipIndex)
		{
			const TextureMipLevelData& mipLevel = loadResult.mipLevels[mipIndex];
			const KTX_error_code setImageResult = ktxTexture_SetImageFromMemory(
			    reinterpret_cast<ktxTexture*>(textureHandle.texture),
			    mipIndex,
			    0,
			    0,
			    mipLevel.data.data(),
			    mipLevel.data.size());
			if (setImageResult != KTX_SUCCESS)
			{
				outErrorMessage = std::format(
				    "Failed to populate KTX2 mip {} for '{}' (error {})",
				    mipIndex,
				    textureAsset.sourcePath.string(),
				    static_cast<int>(setImageResult));
				return false;
			}
		}

		const std::filesystem::path outputPath = BuildTextureAssetPath(textureAsset.assetId);
		std::error_code errorCode;
		std::filesystem::create_directories(outputPath.parent_path(), errorCode);
		if (errorCode)
		{
			outErrorMessage = "Failed to create cooked texture output directory '" + outputPath.parent_path().string() + "'";
			return false;
		}

		const KTX_error_code writeResult =
		    ktxTexture_WriteToNamedFile(reinterpret_cast<ktxTexture*>(textureHandle.texture), outputPath.string().c_str());
		if (writeResult != KTX_SUCCESS)
		{
			outErrorMessage = std::format(
			    "Failed to write KTX2 texture '{}' (error {})",
			    outputPath.string(),
			    static_cast<int>(writeResult));
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	KtxTextureCooker::TextureColorSpace KtxTextureCooker::ResolveColorSpace(Engine::Assets::CookedTextureSemantic semantic) noexcept
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

	bool KtxTextureCooker::NormalizeSourceTexturePath(
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

	bool KtxTextureCooker::BuildTextureSourceKey(
	    const std::filesystem::path& normalizedSourceTexturePath,
	    TextureColorSpace colorSpace,
	    std::string& outTextureSourceKey,
	    std::string& outErrorMessage)
	{
		std::error_code errorCode;

		const std::filesystem::path& projectRoot = Filesystem::GetProjectPath();
		if (!projectRoot.empty())
		{
			std::filesystem::path relativePath = std::filesystem::relative(normalizedSourceTexturePath, projectRoot, errorCode);
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
			std::filesystem::path relativePath = std::filesystem::relative(normalizedSourceTexturePath, engineRoot, errorCode);
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

	bool KtxTextureCooker::ResolveVkFormat(
	    DXGI_FORMAT dxgiFormat,
	    TextureColorSpace colorSpace,
	    std::uint32_t& outVkFormat,
	    std::string& outErrorMessage)
	{
		switch (dxgiFormat)
		{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				outVkFormat = colorSpace == TextureColorSpace::Srgb ? kVkFormatR8G8B8A8Srgb : kVkFormatR8G8B8A8Unorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_B8G8R8A8_UNORM:
				outVkFormat = colorSpace == TextureColorSpace::Srgb ? kVkFormatB8G8R8A8Srgb : kVkFormatB8G8R8A8Unorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC1_UNORM:
				outVkFormat = colorSpace == TextureColorSpace::Srgb ? kVkFormatBc1RgbaSrgb : kVkFormatBc1RgbaUnorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC2_UNORM:
				outVkFormat = colorSpace == TextureColorSpace::Srgb ? kVkFormatBc2Srgb : kVkFormatBc2Unorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC3_UNORM:
				outVkFormat = colorSpace == TextureColorSpace::Srgb ? kVkFormatBc3Srgb : kVkFormatBc3Unorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC4_UNORM:
				if (colorSpace == TextureColorSpace::Srgb)
				{
					outErrorMessage = "BC4 textures cannot be stored as sRGB KTX2 assets";
					return false;
				}

				outVkFormat = kVkFormatBc4Unorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC4_SNORM:
				if (colorSpace == TextureColorSpace::Srgb)
				{
					outErrorMessage = "BC4 SNORM textures cannot be stored as sRGB KTX2 assets";
					return false;
				}

				outVkFormat = kVkFormatBc4Snorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC5_UNORM:
				if (colorSpace == TextureColorSpace::Srgb)
				{
					outErrorMessage = "BC5 textures cannot be stored as sRGB KTX2 assets";
					return false;
				}

				outVkFormat = kVkFormatBc5Unorm;
				outErrorMessage.clear();
				return true;

			case DXGI_FORMAT_BC5_SNORM:
				if (colorSpace == TextureColorSpace::Srgb)
				{
					outErrorMessage = "BC5 SNORM textures cannot be stored as sRGB KTX2 assets";
					return false;
				}

				outVkFormat = kVkFormatBc5Snorm;
				outErrorMessage.clear();
				return true;

			default:
				outErrorMessage = std::format("Unsupported source texture DXGI format {} for KTX2 cooking", static_cast<int>(dxgiFormat));
				return false;
		}
	}
}