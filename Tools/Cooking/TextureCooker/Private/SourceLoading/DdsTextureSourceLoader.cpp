#include "PCH.h"

#include "SourceLoading/TextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoadStages.h"

#include "Core/Public/Diagnostics/Error.h"

#include <algorithm>
#include <cstring>
#include <format>

struct DdsPixelFormat
{
	std::uint32_t size = 0;
	std::uint32_t flags = 0;
	std::uint32_t fourCC = 0;
	std::uint32_t rgbBitCount = 0;
	std::uint32_t rBitMask = 0;
	std::uint32_t gBitMask = 0;
	std::uint32_t bBitMask = 0;
	std::uint32_t aBitMask = 0;
};

struct DdsHeader
{
	std::uint32_t size = 0;
	std::uint32_t flags = 0;
	std::uint32_t height = 0;
	std::uint32_t width = 0;
	std::uint32_t pitchOrLinearSize = 0;
	std::uint32_t depth = 0;
	std::uint32_t mipMapCount = 0;
	std::uint32_t reserved1[11] = {};
	DdsPixelFormat pixelFormat;
	std::uint32_t caps = 0;
	std::uint32_t caps2 = 0;
	std::uint32_t caps3 = 0;
	std::uint32_t caps4 = 0;
	std::uint32_t reserved2 = 0;
};

struct DdsHeaderDx10
{
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
	std::uint32_t resourceDimension = 0;
	std::uint32_t miscFlag = 0;
	std::uint32_t arraySize = 0;
	std::uint32_t miscFlags2 = 0;
};

static_assert(sizeof(DdsPixelFormat) == 32);
static_assert(sizeof(DdsHeader) == 124);
static_assert(sizeof(DdsHeaderDx10) == 20);

static constexpr std::uint32_t kDdsMagic = 0x20534444u;
static constexpr std::uint32_t kPixelFormatFlagFourCc = 0x4u;
static constexpr std::uint32_t kPixelFormatFlagRgb = 0x40u;
static constexpr std::uint32_t kCaps2Cubemap = 0x200u;
static constexpr std::uint32_t kCaps2CubemapAllFaces = 0xfc00u;
static constexpr std::uint32_t kDx10MiscFlagTextureCube = 0x4u;
static constexpr std::uint32_t kResourceDimensionTexture2D = 3u;

static constexpr std::uint32_t MakeFourCc(char a, char b, char c, char d) noexcept
{
	return static_cast<std::uint32_t>(static_cast<unsigned char>(a)) | (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 8u)
	    | (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 16u)
	    | (static_cast<std::uint32_t>(static_cast<unsigned char>(d)) << 24u);
}

static DdsHeader ReadHeader(const std::vector<std::uint8_t>& fileBytes)
{
	if (fileBytes.size() < sizeof(kDdsMagic) + sizeof(DdsHeader))
	{
		throw Diagnostics::Error("DDS file is too small to contain a complete header.");
	}

	std::uint32_t magic = 0;
	std::memcpy(&magic, fileBytes.data(), sizeof(magic));
	if (magic != kDdsMagic)
	{
		throw Diagnostics::Error("File does not contain a DDS magic header.");
	}

	DdsHeader header;
	std::memcpy(&header, fileBytes.data() + sizeof(kDdsMagic), sizeof(header));
	return header;
}

static bool HasDx10Header(const DdsHeader& header) noexcept
{
	return (header.pixelFormat.flags & kPixelFormatFlagFourCc) != 0 && header.pixelFormat.fourCC == MakeFourCc('D', 'X', '1', '0');
}

static DdsHeaderDx10 ReadDx10Header(const std::vector<std::uint8_t>& fileBytes)
{
	if (fileBytes.size() < sizeof(kDdsMagic) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10))
	{
		throw Diagnostics::Error("DDS file is missing its DX10 header.");
	}

	DdsHeaderDx10 dx10Header;
	std::memcpy(&dx10Header, fileBytes.data() + sizeof(kDdsMagic) + sizeof(DdsHeader), sizeof(dx10Header));
	return dx10Header;
}

static bool IsCubemap(const DdsHeader& header, const DdsHeaderDx10* dx10Header) noexcept
{
	if (dx10Header != nullptr && (dx10Header->miscFlag & kDx10MiscFlagTextureCube) != 0)
	{
		return true;
	}

	return (header.caps2 & kCaps2Cubemap) != 0;
}

static void ValidateHeader(const DdsHeader& header, const DdsHeaderDx10* dx10Header, const std::filesystem::path& resolvedPath)
{
	if (header.size != sizeof(DdsHeader) || header.pixelFormat.size != sizeof(DdsPixelFormat))
	{
		throw Diagnostics::Error(std::format("DDS texture '{}' has an invalid header layout.", resolvedPath.string()));
	}

	if (header.width == 0 || header.height == 0)
	{
		throw Diagnostics::Error(std::format("DDS texture '{}' has invalid dimensions.", resolvedPath.string()));
	}

	if ((header.caps2 & kCaps2Cubemap) != 0 && (header.caps2 & kCaps2CubemapAllFaces) != kCaps2CubemapAllFaces)
	{
		throw Diagnostics::Error(std::format("DDS cubemap '{}' does not define all six faces.", resolvedPath.string()));
	}

	if (dx10Header != nullptr)
	{
		if (dx10Header->resourceDimension != kResourceDimensionTexture2D)
		{
			throw Diagnostics::Error(std::format("Only 2D DDS textures are supported: '{}'.", resolvedPath.string()));
		}

		if (IsCubemap(header, dx10Header))
		{
			if (dx10Header->arraySize != 6)
			{
				throw Diagnostics::Error(std::format("DDS cubemap '{}' must declare exactly six faces.", resolvedPath.string()));
			}
		}
		else if (dx10Header->arraySize != 1)
		{
			throw Diagnostics::Error(std::format("DDS texture arrays are not supported yet: '{}'.", resolvedPath.string()));
		}
	}
}

static DXGI_FORMAT ResolveDxgiFormat(const DdsHeader& header, const DdsHeaderDx10* dx10Header, const std::filesystem::path& resolvedPath)
{
	if (dx10Header != nullptr)
	{
		if (dx10Header->dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			throw Diagnostics::Error(std::format("DDS texture '{}' declares an unknown DXGI format.", resolvedPath.string()));
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
				throw Diagnostics::Error(std::format("Unsupported DDS FourCC in '{}'.", resolvedPath.string()));
		}
	}

	if ((header.pixelFormat.flags & kPixelFormatFlagRgb) != 0)
	{
		if (header.pixelFormat.rgbBitCount == 32 && header.pixelFormat.rBitMask == 0x000000ffu && header.pixelFormat.gBitMask == 0x0000ff00u
		    && header.pixelFormat.bBitMask == 0x00ff0000u && header.pixelFormat.aBitMask == 0xff000000u)
		{
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		if (header.pixelFormat.rgbBitCount == 32 && header.pixelFormat.rBitMask == 0x00ff0000u && header.pixelFormat.gBitMask == 0x0000ff00u
		    && header.pixelFormat.bBitMask == 0x000000ffu && header.pixelFormat.aBitMask == 0xff000000u)
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
	}

	throw Diagnostics::Error(std::format("Unsupported DDS pixel format in '{}'.", resolvedPath.string()));
}

static std::uint32_t ResolveBitsPerPixel(DXGI_FORMAT format, const std::filesystem::path& resolvedPath)
{
	switch (format)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return 32;
		default:
			throw Diagnostics::Error(
			    std::format(
			        "Uncompressed DDS bit-depth query is unsupported for format {} in '{}'",
			        static_cast<int>(format),
			        resolvedPath.string()));
	}
}

static std::uint32_t ResolveBlockSize(DXGI_FORMAT format, const std::filesystem::path& resolvedPath)
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
			throw Diagnostics::Error(
			    std::format("DDS block-size query is unsupported for format {} in '{}'", static_cast<int>(format), resolvedPath.string()));
	}
}

static std::uint32_t ResolveMipCount(const DdsHeader& header) noexcept
{
	return (std::max) (1u, header.mipMapCount);
}

static std::uint32_t ResolveArraySize(const DdsHeader& header, const DdsHeaderDx10* dx10Header) noexcept
{
	if (IsCubemap(header, dx10Header))
	{
		return 6;
	}

	if (dx10Header != nullptr && dx10Header->arraySize > 0)
	{
		return dx10Header->arraySize;
	}

	return 1;
}

static bool IsBlockCompressed(DXGI_FORMAT format) noexcept
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

static std::uint32_t ComputeRowPitch(DXGI_FORMAT format, std::uint32_t width, const std::filesystem::path& resolvedPath)
{
	if (IsBlockCompressed(format))
	{
		const std::uint32_t blockCountX = (std::max) (1u, (width + 3u) / 4u);
		return blockCountX * ResolveBlockSize(format, resolvedPath);
	}

	return (width * ResolveBitsPerPixel(format, resolvedPath) + 7u) / 8u;
}

static std::uint32_t ComputeSlicePitch(
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

static std::size_t ResolvePixelDataOffset(const DdsHeader& header) noexcept
{
	return sizeof(kDdsMagic) + sizeof(DdsHeader) + (HasDx10Header(header) ? sizeof(DdsHeaderDx10) : 0u);
}

static TextureLoadResult BuildLoadResult(
    const std::vector<std::uint8_t>& fileBytes,
    const DdsHeader& header,
    const DdsHeaderDx10* dx10Header,
    DXGI_FORMAT dxgiFormat,
    const std::filesystem::path& resolvedPath)
{
	TextureLoadResult loadResult;
	loadResult.width = header.width;
	loadResult.height = header.height;
	loadResult.arraySize = ResolveArraySize(header, dx10Header);
	loadResult.dimension = IsCubemap(header, dx10Header) ? TextureResourceDimension::TextureCube : TextureResourceDimension::Texture2D;
	loadResult.dxgiFormat = dxgiFormat;
	loadResult.formatIntent = TextureFormatIntent::Unknown;
	loadResult.arraySlices.resize(loadResult.arraySize);
	for (TextureArraySliceData& arraySlice : loadResult.arraySlices)
	{
		arraySlice.reserve(ResolveMipCount(header));
	}

	std::size_t byteOffset = ResolvePixelDataOffset(header);
	for (std::uint32_t arraySliceIndex = 0; arraySliceIndex < loadResult.arraySize; ++arraySliceIndex)
	{
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
				throw Diagnostics::Error(
				    std::format(
				        "DDS texture '{}' ended before slice {} mip {} could be read",
				        resolvedPath.string(),
				        arraySliceIndex,
				        mipIndex));
			}

			mipLevel.data.assign(
			    fileBytes.begin() + static_cast<std::ptrdiff_t>(byteOffset),
			    fileBytes.begin() + static_cast<std::ptrdiff_t>(byteOffset + mipLevel.slicePitch));
			loadResult.arraySlices[arraySliceIndex].push_back(std::move(mipLevel));

			byteOffset += loadResult.arraySlices[arraySliceIndex].back().slicePitch;
			mipWidth = (std::max) (1u, mipWidth >> 1u);
			mipHeight = (std::max) (1u, mipHeight >> 1u);
		}
	}

	return loadResult;
}

TextureLoadResult LoadDdsTextureSource(const std::filesystem::path& sourcePath)
{
	const TextureSourceFile sourceFile = ReadTextureSourceFile(sourcePath);
	const DdsHeader header = ReadHeader(sourceFile.Bytes);

	const bool hasDx10Header = HasDx10Header(header);
	const DdsHeaderDx10 dx10Header = hasDx10Header ? ReadDx10Header(sourceFile.Bytes) : DdsHeaderDx10{};

	const DdsHeaderDx10* dx10HeaderPtr = hasDx10Header ? &dx10Header : nullptr;
	ValidateHeader(header, dx10HeaderPtr, sourceFile.ResolvedPath);
	const DXGI_FORMAT dxgiFormat = ResolveDxgiFormat(header, dx10HeaderPtr, sourceFile.ResolvedPath);

	return BuildLoadResult(sourceFile.Bytes, header, dx10HeaderPtr, dxgiFormat, sourceFile.ResolvedPath);
}
