#include "PCH.h"

#include "D3D12/Textures/DdsTextureLoader.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <limits>

static const auto g_ddsTextureLoaderLogger = Logging::GetOrCreateLogger("RHI.Textures");

bool DdsTextureLoader::SupportsExtension(std::wstring_view extension) const noexcept
{
	return extension == L".dds";
}

TextureLoadResult DdsTextureLoader::Load(const std::filesystem::path& fileName) const
{
	const std::filesystem::path resolvedPath = Filesystem::ResolveAssetPathValidated(fileName, AssetType::Texture);
	std::vector<std::uint8_t> fileBytes;
	std::string readErrorMessage;
	if (!Files::TryReadAllBytes(resolvedPath, fileBytes, readErrorMessage))
	{
		Diagnostics::Fail(g_ddsTextureLoaderLogger, __FILE__, __LINE__, std::format("DdsTextureLoader: {}", readErrorMessage));
		return {};
	}

	const DdsHeader header = ReadHeader(fileBytes);
	const bool hasDx10Header = HasDx10Header(header);
	const DdsHeaderDx10 dx10Header = hasDx10Header ? ReadDx10Header(fileBytes) : DdsHeaderDx10{};
	const DdsHeaderDx10* dx10HeaderPtr = hasDx10Header ? &dx10Header : nullptr;

	ValidateHeader(header, dx10HeaderPtr, resolvedPath);
	const DXGI_FORMAT dxgiFormat = ResolveDxgiFormat(header, dx10HeaderPtr, resolvedPath);
	return BuildLoadResult(fileBytes, header, dxgiFormat, resolvedPath);
}

DdsTextureLoader::DdsHeader DdsTextureLoader::ReadHeader(const std::vector<std::uint8_t>& fileBytes)
{
	if (fileBytes.size() < sizeof(kDdsMagic) + sizeof(DdsHeader))
	{
		Diagnostics::Fail(
		    g_ddsTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    "DdsTextureLoader: DDS file is too small to contain a valid header.");
		return {};
	}

	std::uint32_t magic = 0;
	std::memcpy(&magic, fileBytes.data(), sizeof(magic));
	if (magic != kDdsMagic)
	{
		Diagnostics::Fail(
		    g_ddsTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    "DdsTextureLoader: File does not contain a DDS magic header.");
		return {};
	}

	DdsHeader header;
	std::memcpy(&header, fileBytes.data() + sizeof(kDdsMagic), sizeof(header));
	return header;
}

bool DdsTextureLoader::HasDx10Header(const DdsHeader& header) noexcept
{
	return (header.pixelFormat.flags & kPixelFormatFlagFourCc) != 0 && header.pixelFormat.fourCC == MakeFourCc('D', 'X', '1', '0');
}

DdsTextureLoader::DdsHeaderDx10 DdsTextureLoader::ReadDx10Header(const std::vector<std::uint8_t>& fileBytes)
{
	if (fileBytes.size() < sizeof(kDdsMagic) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10))
	{
		Diagnostics::Fail(
		    g_ddsTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    "DdsTextureLoader: DDS file is missing the required DX10 header.");
		return {};
	}

	DdsHeaderDx10 dx10Header;
	std::memcpy(&dx10Header, fileBytes.data() + sizeof(kDdsMagic) + sizeof(DdsHeader), sizeof(dx10Header));
	return dx10Header;
}

void DdsTextureLoader::ValidateHeader(const DdsHeader& header, const DdsHeaderDx10* dx10Header, const std::filesystem::path& resolvedPath)
{
	if (header.size != sizeof(DdsHeader) || header.pixelFormat.size != sizeof(DdsPixelFormat))
	{
		Diagnostics::Fail(
		    g_ddsTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("DdsTextureLoader: '{}' has an invalid DDS header layout", resolvedPath.string()));
		return;
	}

	if (header.width == 0 || header.height == 0)
	{
		Diagnostics::Fail(
		    g_ddsTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("DdsTextureLoader: '{}' has invalid dimensions", resolvedPath.string()));
		return;
	}

	if ((header.caps2 & kCaps2Cubemap) != 0)
	{
		Diagnostics::Fail(
		    g_ddsTextureLoaderLogger,
		    __FILE__,
		    __LINE__,
		    std::format("DdsTextureLoader: Cubemap DDS textures are not supported yet: '{}'", resolvedPath.string()));
		return;
	}

	if (dx10Header != nullptr)
	{
		if (dx10Header->resourceDimension != kResourceDimensionTexture2D)
		{
			Diagnostics::Fail(
			    g_ddsTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format("DdsTextureLoader: Only 2D DDS textures are supported: '{}'", resolvedPath.string()));
			return;
		}

		if (dx10Header->arraySize != 1)
		{
			Diagnostics::Fail(
			    g_ddsTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format("DdsTextureLoader: DDS texture arrays are not supported yet: '{}'", resolvedPath.string()));
			return;
		}
	}
}

DXGI_FORMAT DdsTextureLoader::ResolveDxgiFormat(
    const DdsHeader& header,
    const DdsHeaderDx10* dx10Header,
    const std::filesystem::path& resolvedPath)
{
	if (dx10Header != nullptr)
	{
		if (dx10Header->dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			Diagnostics::Fail(
			    g_ddsTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format("DdsTextureLoader: '{}' declares an unknown DXGI format", resolvedPath.string()));
			return DXGI_FORMAT_UNKNOWN;
		}

		return dx10Header->dxgiFormat;
	}

	if ((header.pixelFormat.flags & kPixelFormatFlagFourCc) != 0)
	{
		switch (header.pixelFormat.fourCC)
		{
			case MakeFourCc('D', 'X', 'T', '1'):
				return DXGI_FORMAT_BC1_UNORM;
			case MakeFourCc('D', 'X', 'T', '3'):
				return DXGI_FORMAT_BC2_UNORM;
			case MakeFourCc('D', 'X', 'T', '5'):
				return DXGI_FORMAT_BC3_UNORM;
			case MakeFourCc('A', 'T', 'I', '1'):
				return DXGI_FORMAT_BC4_UNORM;
			case MakeFourCc('A', 'T', 'I', '2'):
				return DXGI_FORMAT_BC5_UNORM;
			case MakeFourCc('B', 'C', '4', 'U'):
				return DXGI_FORMAT_BC4_UNORM;
			case MakeFourCc('B', 'C', '4', 'S'):
				return DXGI_FORMAT_BC4_SNORM;
			case MakeFourCc('B', 'C', '5', 'U'):
				return DXGI_FORMAT_BC5_UNORM;
			case MakeFourCc('B', 'C', '5', 'S'):
				return DXGI_FORMAT_BC5_SNORM;
			default:
				Diagnostics::Fail(
				    g_ddsTextureLoaderLogger,
				    __FILE__,
				    __LINE__,
				    std::format(
				        "DdsTextureLoader: Unsupported DDS FourCC '{}' in '{}'",
				        std::string(
				            {static_cast<char>(header.pixelFormat.fourCC & 0xFFu),
				             static_cast<char>((header.pixelFormat.fourCC >> 8u) & 0xFFu),
				             static_cast<char>((header.pixelFormat.fourCC >> 16u) & 0xFFu),
				             static_cast<char>((header.pixelFormat.fourCC >> 24u) & 0xFFu)}),
				        resolvedPath.string()));
				return DXGI_FORMAT_UNKNOWN;
		}
	}

	if ((header.pixelFormat.flags & kPixelFormatFlagRgb) != 0)
	{
		if (header.pixelFormat.rgbBitCount == 32 && header.pixelFormat.rBitMask == 0x000000ffu &&
		    header.pixelFormat.gBitMask == 0x0000ff00u && header.pixelFormat.bBitMask == 0x00ff0000u &&
		    header.pixelFormat.aBitMask == 0xff000000u)
		{
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		if (header.pixelFormat.rgbBitCount == 32 && header.pixelFormat.rBitMask == 0x00ff0000u &&
		    header.pixelFormat.gBitMask == 0x0000ff00u && header.pixelFormat.bBitMask == 0x000000ffu &&
		    header.pixelFormat.aBitMask == 0xff000000u)
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
	}

	Diagnostics::Fail(
	    g_ddsTextureLoaderLogger,
	    __FILE__,
	    __LINE__,
	    std::format("DdsTextureLoader: Unsupported DDS pixel format in '{}'", resolvedPath.string()));
	return DXGI_FORMAT_UNKNOWN;
}

std::uint32_t DdsTextureLoader::ResolveBitsPerPixel(DXGI_FORMAT format, const std::filesystem::path& resolvedPath)
{
	switch (format)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return 32;
		default:
			Diagnostics::Fail(
			    g_ddsTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "DdsTextureLoader: Uncompressed bit-depth query is unsupported for format {} in '{}'",
			        static_cast<int>(format),
			        resolvedPath.string()));
			return 0;
	}
}

std::uint32_t DdsTextureLoader::ResolveBlockSize(DXGI_FORMAT format, const std::filesystem::path& resolvedPath)
{
	switch (format)
	{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 8;
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return 16;
		default:
			Diagnostics::Fail(
			    g_ddsTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format(
			        "DdsTextureLoader: Block-size query is unsupported for format {} in '{}'",
			        static_cast<int>(format),
			        resolvedPath.string()));
			return 0;
	}
}

std::uint32_t DdsTextureLoader::ResolveMipCount(const DdsHeader& header) noexcept
{
	return (std::max) (1u, header.mipMapCount);
}

bool DdsTextureLoader::IsBlockCompressed(DXGI_FORMAT format) noexcept
{
	switch (format)
	{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return true;
		default:
			return false;
	}
}

std::uint32_t DdsTextureLoader::ComputeRowPitch(DXGI_FORMAT format, std::uint32_t width, const std::filesystem::path& resolvedPath)
{
	if (IsBlockCompressed(format))
	{
		const std::uint32_t blockCountX = (std::max) (1u, (width + 3u) / 4u);
		return blockCountX * ResolveBlockSize(format, resolvedPath);
	}

	return (width * ResolveBitsPerPixel(format, resolvedPath) + 7u) / 8u;
}

std::uint32_t DdsTextureLoader::ComputeSlicePitch(
    DXGI_FORMAT format,
    std::uint32_t width,
    std::uint32_t height,
    const std::filesystem::path& resolvedPath)
{
	if (IsBlockCompressed(format))
	{
		const std::uint32_t blockCountY = (std::max) (1u, (height + 3u) / 4u);
		return ComputeRowPitch(format, width, resolvedPath) * blockCountY;
	}

	return ComputeRowPitch(format, width, resolvedPath) * height;
}

std::size_t DdsTextureLoader::ResolvePixelDataOffset(const DdsHeader& header) noexcept
{
	return sizeof(kDdsMagic) + sizeof(DdsHeader) + (HasDx10Header(header) ? sizeof(DdsHeaderDx10) : 0u);
}

TextureLoadResult DdsTextureLoader::BuildLoadResult(
    const std::vector<std::uint8_t>& fileBytes,
    const DdsHeader& header,
    DXGI_FORMAT dxgiFormat,
    const std::filesystem::path& resolvedPath)
{
	TextureLoadResult loadResult;
	loadResult.width = header.width;
	loadResult.height = header.height;
	loadResult.arraySize = 1;
	loadResult.dimension = TextureResourceDimension::Texture2D;
	loadResult.dxgiFormat = dxgiFormat;
	loadResult.formatIntent = TextureFormatIntent::Unknown;
	loadResult.arraySlices.resize(1);
	loadResult.arraySlices.front().mipLevels.reserve(ResolveMipCount(header));

	std::size_t byteOffset = ResolvePixelDataOffset(header);
	std::uint32_t mipWidth = header.width;
	std::uint32_t mipHeight = header.height;

	for (std::uint32_t mipIndex = 0; mipIndex < ResolveMipCount(header); ++mipIndex)
	{
		TextureMipLevelData mipLevel;
		mipLevel.width = (std::max) (1u, mipWidth);
		mipLevel.height = (std::max) (1u, mipHeight);
		mipLevel.rowPitch = ComputeRowPitch(dxgiFormat, mipLevel.width, resolvedPath);
		mipLevel.slicePitch = ComputeSlicePitch(dxgiFormat, mipLevel.width, mipLevel.height, resolvedPath);

		if (byteOffset + mipLevel.slicePitch > fileBytes.size())
		{
			Diagnostics::Fail(
			    g_ddsTextureLoaderLogger,
			    __FILE__,
			    __LINE__,
			    std::format("DdsTextureLoader: '{}' ended before mip {} could be read", resolvedPath.string(), mipIndex));
			return {};
		}

		mipLevel.data.assign(
		    fileBytes.begin() + static_cast<std::ptrdiff_t>(byteOffset),
		    fileBytes.begin() + static_cast<std::ptrdiff_t>(byteOffset + mipLevel.slicePitch));
		loadResult.arraySlices.front().mipLevels.push_back(std::move(mipLevel));

		byteOffset += loadResult.arraySlices.front().mipLevels.back().slicePitch;
		mipWidth = (std::max) (1u, mipWidth >> 1u);
		mipHeight = (std::max) (1u, mipHeight >> 1u);
	}

	return loadResult;
}
