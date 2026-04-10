#include "PCH.h"

#include "D3D12/Textures/KtxTextureLoader.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Log.h"

#include <ktx.h>

#include <algorithm>
#include <format>

TextureLoadResult KtxTextureLoader::Load(const std::filesystem::path& fileName)
{
	TextureLoadResult result;

	const auto resolvedPathResult = Filesystem::ResolveAssetPathNormalized(fileName, AssetType::Texture);
	if (!resolvedPathResult)
	{
		LOG_ERROR(std::format("KtxTextureLoader: Failed to resolve '{}'", fileName.string()));
		return result;
	}

	const std::filesystem::path& resolvedPath = *resolvedPathResult;
	ktxTexture2* rawTexture = nullptr;
	const KTX_error_code createResult =
	    ktxTexture2_CreateFromNamedFile(resolvedPath.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &rawTexture);
	auto destroyTexture = [](ktxTexture2* texture)
	{
		if (texture != nullptr)
		{
			ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(texture));
		}
	};
	std::unique_ptr<ktxTexture2, decltype(destroyTexture)> texture(rawTexture, destroyTexture);
	if (createResult != KTX_SUCCESS || !texture)
	{
		LOG_ERROR(std::format(
		    "KtxTextureLoader: Failed to parse '{}' ({})",
		    resolvedPath.string(),
		    ktxErrorString(createResult)));
		return result;
	}

	std::string errorMessage;
	if (!ValidateTextureShape(
		        texture->numDimensions,
		        texture->isArray == KTX_TRUE,
		        texture->isCubemap == KTX_TRUE,
		        texture->numLayers,
		        texture->numFaces,
		        resolvedPath,
		        errorMessage))
	{
		LOG_ERROR("KtxTextureLoader: " + errorMessage);
		return {};
	}

	if (ktxTexture_NeedsTranscoding(reinterpret_cast<ktxTexture*>(texture.get())))
	{
		LOG_ERROR(
		    std::format(
		        "KtxTextureLoader: '{}' requires Basis transcoding, which is unsupported in the runtime cooked-texture path",
		        resolvedPath.string()));
		return {};
	}

	if (!ResolveDxgiFormat(texture->vkFormat, result.dxgiFormat, result.formatIntent, errorMessage))
	{
		LOG_ERROR(std::format("KtxTextureLoader: Unsupported cooked texture '{}' - {}", resolvedPath.string(), errorMessage));
		return {};
	}

	result.width = texture->baseWidth;
	result.height = texture->baseHeight;
	result.mipLevels.reserve(texture->numLevels);

	for (std::uint32_t mipIndex = 0; mipIndex < texture->numLevels; ++mipIndex)
	{
		ktx_size_t imageOffset = 0;
		const KTX_error_code offsetResult =
		    ktxTexture_GetImageOffset(reinterpret_cast<ktxTexture*>(texture.get()), mipIndex, 0, 0, &imageOffset);
		if (offsetResult != KTX_SUCCESS)
		{
			LOG_ERROR(std::format(
			    "KtxTextureLoader: Failed to query mip {} offset for '{}' ({})",
			    mipIndex,
			    resolvedPath.string(),
			    ktxErrorString(offsetResult)));
			return {};
		}

		const ktx_size_t imageSize = ktxTexture_GetImageSize(reinterpret_cast<ktxTexture*>(texture.get()), mipIndex);
		if (!ValidateMipPayloadRange(
		        static_cast<std::size_t>(imageOffset),
		        static_cast<std::size_t>(imageSize),
		        static_cast<std::size_t>(texture->dataSize),
		        resolvedPath,
		        errorMessage))
		{
			LOG_ERROR("KtxTextureLoader: " + errorMessage);
			return {};
		}

		TextureMipLevelData mipLevel;
		mipLevel.width = (std::max)(1u, texture->baseWidth >> mipIndex);
		mipLevel.height = (std::max)(1u, texture->baseHeight >> mipIndex);
		mipLevel.rowPitch = ResolveRowPitch(result.dxgiFormat, mipLevel.width);
		mipLevel.slicePitch = ResolveSlicePitch(result.dxgiFormat, mipLevel.width, mipLevel.height);
		mipLevel.data.assign(
		    texture->pData + imageOffset,
		    texture->pData + imageOffset + imageSize);

		if (mipLevel.slicePitch != imageSize)
		{
			LOG_ERROR(std::format(
			    "KtxTextureLoader: Mip {} for '{}' reported {} bytes but Sparkle expected {} bytes for format {}",
			    mipIndex,
			    resolvedPath.string(),
			    static_cast<std::size_t>(imageSize),
			    mipLevel.slicePitch,
			    static_cast<int>(result.dxgiFormat)));
			return {};
		}

		result.mipLevels.push_back(std::move(mipLevel));
	}

	return result;
}

bool KtxTextureLoader::SupportsExtension(std::wstring_view extension) noexcept
{
	return extension == L".ktx2";
}

bool KtxTextureLoader::ResolveDxgiFormat(
	std::uint32_t vkFormat,
	DXGI_FORMAT& outDxgiFormat,
	TextureFormatIntent& outFormatIntent,
	std::string& outErrorMessage)
{
	switch (vkFormat)
	{
		case kVkFormatR8G8B8A8Unorm:
			outDxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatR8G8B8A8Srgb:
			outDxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			outFormatIntent = TextureFormatIntent::ColorSrgb;
			outErrorMessage.clear();
			return true;

		case kVkFormatB8G8R8A8Unorm:
			outDxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatB8G8R8A8Srgb:
			outDxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			outFormatIntent = TextureFormatIntent::ColorSrgb;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc1RgbaUnorm:
			outDxgiFormat = DXGI_FORMAT_BC1_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc1RgbaSrgb:
			outDxgiFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
			outFormatIntent = TextureFormatIntent::ColorSrgb;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc2Unorm:
			outDxgiFormat = DXGI_FORMAT_BC2_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc2Srgb:
			outDxgiFormat = DXGI_FORMAT_BC2_UNORM_SRGB;
			outFormatIntent = TextureFormatIntent::ColorSrgb;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc3Unorm:
			outDxgiFormat = DXGI_FORMAT_BC3_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc3Srgb:
			outDxgiFormat = DXGI_FORMAT_BC3_UNORM_SRGB;
			outFormatIntent = TextureFormatIntent::ColorSrgb;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc4Unorm:
			outDxgiFormat = DXGI_FORMAT_BC4_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc4Snorm:
			outDxgiFormat = DXGI_FORMAT_BC4_SNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc5Unorm:
			outDxgiFormat = DXGI_FORMAT_BC5_UNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		case kVkFormatBc5Snorm:
			outDxgiFormat = DXGI_FORMAT_BC5_SNORM;
			outFormatIntent = TextureFormatIntent::DataLinear;
			outErrorMessage.clear();
			return true;

		default:
			outErrorMessage = std::format("vkFormat {} is not supported by the runtime KTX2 loader", vkFormat);
			return false;
	}
}

bool KtxTextureLoader::ValidateTextureShape(
	std::uint32_t numDimensions,
	bool isArray,
	bool isCubemap,
	std::uint32_t numLayers,
	std::uint32_t numFaces,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	if (numDimensions != 2)
	{
		outErrorMessage = std::format(
		    "'{}' declares {} dimensions, but Sparkle runtime only supports 2D cooked KTX2 textures",
		    resolvedPath.string(),
		    numDimensions);
		return false;
	}

	if (isArray || numLayers != 1)
	{
		outErrorMessage = std::format(
		    "'{}' is a texture array, which is unsupported in the runtime cooked KTX2 path",
		    resolvedPath.string());
		return false;
	}

	if (isCubemap || numFaces != 1)
	{
		outErrorMessage = std::format(
		    "'{}' is a cubemap or multi-face texture, which is unsupported in the runtime cooked KTX2 path",
		    resolvedPath.string());
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool KtxTextureLoader::ValidateMipPayloadRange(
	std::size_t byteOffset,
	std::size_t byteCount,
	std::size_t dataSize,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	if (byteOffset > dataSize || byteCount > dataSize - byteOffset)
	{
		outErrorMessage = std::format(
		    "'{}' contains a KTX2 mip payload outside the loaded image data range",
		    resolvedPath.string());
		return false;
	}

	outErrorMessage.clear();
	return true;
}

std::uint32_t KtxTextureLoader::ResolveRowPitch(DXGI_FORMAT dxgiFormat, std::uint32_t width) noexcept
{
	if (IsBlockCompressed(dxgiFormat))
	{
		return (std::max)(1u, (width + 3u) / 4u) * ResolveBlockSize(dxgiFormat);
	}

	return width * ResolveBytesPerPixel(dxgiFormat);
}

std::uint32_t KtxTextureLoader::ResolveSlicePitch(DXGI_FORMAT dxgiFormat, std::uint32_t width, std::uint32_t height) noexcept
{
	if (IsBlockCompressed(dxgiFormat))
	{
		return ResolveRowPitch(dxgiFormat, width) * (std::max)(1u, (height + 3u) / 4u);
	}

	return ResolveRowPitch(dxgiFormat, width) * height;
}

bool KtxTextureLoader::IsBlockCompressed(DXGI_FORMAT dxgiFormat) noexcept
{
	switch (dxgiFormat)
	{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return true;

		default:
			return false;
	}
}

std::uint32_t KtxTextureLoader::ResolveBlockSize(DXGI_FORMAT dxgiFormat) noexcept
{
	switch (dxgiFormat)
	{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 8;

		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return 16;

		default:
			return 0;
	}
}

std::uint32_t KtxTextureLoader::ResolveBytesPerPixel(DXGI_FORMAT dxgiFormat) noexcept
{
	switch (dxgiFormat)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return 4;

		default:
			return 0;
	}
}