#include "PCH.h"

#include "SourceLoading/DdsTextureSourceLoader.h"

#include "SourceLoading/TextureSourceLoaderUtils.h"

#include <algorithm>
#include <cstring>
#include <format>

bool DdsTextureSourceLoader::SupportsFormat(TextureSourceFormat format) const noexcept
{
	return format == TextureSourceFormat::Dds;
}

TextureLoadResult DdsTextureSourceLoader::Load(const std::filesystem::path& sourcePath, std::string& outErrorMessage) const
{
	std::filesystem::path resolvedPath;
	std::vector<std::uint8_t> fileBytes;
	if (!TextureSourceLoaderUtils::TryReadSourceBytes(sourcePath, resolvedPath, fileBytes, outErrorMessage))
	{
		return {};
	}

	const DdsHeader header = ReadHeader(fileBytes, outErrorMessage);
	if (!outErrorMessage.empty())
	{
		return {};
	}

	const bool hasDx10Header = HasDx10Header(header);
	const DdsHeaderDx10 dx10Header = hasDx10Header ? ReadDx10Header(fileBytes, outErrorMessage) : DdsHeaderDx10{};
	if (!outErrorMessage.empty())
	{
		return {};
	}

	const DdsHeaderDx10* dx10HeaderPtr = hasDx10Header ? &dx10Header : nullptr;
	if (!ValidateHeader(header, dx10HeaderPtr, resolvedPath, outErrorMessage))
	{
		return {};
	}

	const DXGI_FORMAT dxgiFormat = ResolveDxgiFormat(header, dx10HeaderPtr, resolvedPath, outErrorMessage);
	if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		return {};
	}

	return BuildLoadResult(fileBytes, header, dx10HeaderPtr, dxgiFormat, resolvedPath, outErrorMessage);
}

DdsTextureSourceLoader::DdsHeader DdsTextureSourceLoader::ReadHeader(const std::vector<std::uint8_t>& fileBytes, std::string& outErrorMessage)
{
	if (fileBytes.size() < sizeof(kDdsMagic) + sizeof(DdsHeader))
	{
		outErrorMessage = "DDS file is too small to contain a valid header.";
		return {};
	}

	std::uint32_t magic = 0;
	std::memcpy(&magic, fileBytes.data(), sizeof(magic));
	if (magic != kDdsMagic)
	{
		outErrorMessage = "File does not contain a DDS magic header.";
		return {};
	}

	DdsHeader header;
	std::memcpy(&header, fileBytes.data() + sizeof(kDdsMagic), sizeof(header));
	outErrorMessage.clear();
	return header;
}

bool DdsTextureSourceLoader::HasDx10Header(const DdsHeader& header) noexcept
{
	return (header.pixelFormat.flags & kPixelFormatFlagFourCc) != 0 && header.pixelFormat.fourCC == MakeFourCc('D', 'X', '1', '0');
}

DdsTextureSourceLoader::DdsHeaderDx10 DdsTextureSourceLoader::ReadDx10Header(
	const std::vector<std::uint8_t>& fileBytes,
	std::string& outErrorMessage)
{
	if (fileBytes.size() < sizeof(kDdsMagic) + sizeof(DdsHeader) + sizeof(DdsHeaderDx10))
	{
		outErrorMessage = "DDS file is missing the required DX10 header.";
		return {};
	}

	DdsHeaderDx10 dx10Header;
	std::memcpy(&dx10Header, fileBytes.data() + sizeof(kDdsMagic) + sizeof(DdsHeader), sizeof(dx10Header));
	outErrorMessage.clear();
	return dx10Header;
}

bool DdsTextureSourceLoader::ValidateHeader(
	const DdsHeader& header,
	const DdsHeaderDx10* dx10Header,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	if (header.size != sizeof(DdsHeader) || header.pixelFormat.size != sizeof(DdsPixelFormat))
	{
		outErrorMessage = std::format("DDS texture '{}' has an invalid header layout", resolvedPath.string());
		return false;
	}

	if (header.width == 0 || header.height == 0)
	{
		outErrorMessage = std::format("DDS texture '{}' has invalid dimensions", resolvedPath.string());
		return false;
	}

	if ((header.caps2 & kCaps2Cubemap) != 0 && (header.caps2 & kCaps2CubemapAllFaces) != kCaps2CubemapAllFaces)
	{
		outErrorMessage = std::format("DDS cubemap '{}' does not define all six faces", resolvedPath.string());
		return false;
	}

	if (dx10Header != nullptr)
	{
		if (dx10Header->resourceDimension != kResourceDimensionTexture2D)
		{
			outErrorMessage = std::format("Only 2D DDS textures are supported: '{}'", resolvedPath.string());
			return false;
		}

		if (IsCubemap(header, dx10Header))
		{
			if (dx10Header->arraySize != 6)
			{
				outErrorMessage = std::format("DDS cubemap '{}' must declare exactly six faces", resolvedPath.string());
				return false;
			}
		}
		else if (dx10Header->arraySize != 1)
		{
			outErrorMessage = std::format("DDS texture arrays are not supported yet: '{}'", resolvedPath.string());
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}

DXGI_FORMAT DdsTextureSourceLoader::ResolveDxgiFormat(
	const DdsHeader& header,
	const DdsHeaderDx10* dx10Header,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	if (dx10Header != nullptr)
	{
		if (dx10Header->dxgiFormat == DXGI_FORMAT_UNKNOWN)
		{
			outErrorMessage = std::format("DDS texture '{}' declares an unknown DXGI format", resolvedPath.string());
			return DXGI_FORMAT_UNKNOWN;
		}

		outErrorMessage.clear();
		return dx10Header->dxgiFormat;
	}

	if ((header.pixelFormat.flags & kPixelFormatFlagFourCc) != 0)
	{
		switch (header.pixelFormat.fourCC)
		{
			case MakeFourCc('D', 'X', 'T', '1'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC1_UNORM;
			case MakeFourCc('D', 'X', 'T', '3'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC2_UNORM;
			case MakeFourCc('D', 'X', 'T', '5'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC3_UNORM;
			case MakeFourCc('A', 'T', 'I', '1'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC4_UNORM;
			case MakeFourCc('A', 'T', 'I', '2'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC5_UNORM;
			case MakeFourCc('B', 'C', '4', 'U'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC4_UNORM;
			case MakeFourCc('B', 'C', '4', 'S'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC4_SNORM;
			case MakeFourCc('B', 'C', '5', 'U'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC5_UNORM;
			case MakeFourCc('B', 'C', '5', 'S'):
				outErrorMessage.clear();
				return DXGI_FORMAT_BC5_SNORM;
			default:
				outErrorMessage = std::format("Unsupported DDS FourCC in '{}'", resolvedPath.string());
				return DXGI_FORMAT_UNKNOWN;
		}
	}

	if ((header.pixelFormat.flags & kPixelFormatFlagRgb) != 0)
	{
		if (header.pixelFormat.rgbBitCount == 32 && header.pixelFormat.rBitMask == 0x000000ffu &&
		    header.pixelFormat.gBitMask == 0x0000ff00u && header.pixelFormat.bBitMask == 0x00ff0000u &&
		    header.pixelFormat.aBitMask == 0xff000000u)
		{
			outErrorMessage.clear();
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		if (header.pixelFormat.rgbBitCount == 32 && header.pixelFormat.rBitMask == 0x00ff0000u &&
		    header.pixelFormat.gBitMask == 0x0000ff00u && header.pixelFormat.bBitMask == 0x000000ffu &&
		    header.pixelFormat.aBitMask == 0xff000000u)
		{
			outErrorMessage.clear();
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
	}

	outErrorMessage = std::format("Unsupported DDS pixel format in '{}'", resolvedPath.string());
	return DXGI_FORMAT_UNKNOWN;
}

std::uint32_t DdsTextureSourceLoader::ResolveBitsPerPixel(
	DXGI_FORMAT format,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	switch (format)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			outErrorMessage.clear();
			return 32;
		default:
			outErrorMessage = std::format(
			    "Uncompressed DDS bit-depth query is unsupported for format {} in '{}'",
			    static_cast<int>(format),
			    resolvedPath.string());
			return 0;
	}
}

std::uint32_t DdsTextureSourceLoader::ResolveBlockSize(
	DXGI_FORMAT format,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	switch (format)
	{
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			outErrorMessage.clear();
			return 8;
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			outErrorMessage.clear();
			return 16;
		default:
			outErrorMessage = std::format(
			    "DDS block-size query is unsupported for format {} in '{}'",
			    static_cast<int>(format),
			    resolvedPath.string());
			return 0;
	}
}

std::uint32_t DdsTextureSourceLoader::ResolveMipCount(const DdsHeader& header) noexcept
{
	return (std::max)(1u, header.mipMapCount);
}

bool DdsTextureSourceLoader::IsCubemap(const DdsHeader& header, const DdsHeaderDx10* dx10Header) noexcept
{
	if (dx10Header != nullptr && (dx10Header->miscFlag & kDx10MiscFlagTextureCube) != 0)
	{
		return true;
	}

	return (header.caps2 & kCaps2Cubemap) != 0;
}

std::uint32_t DdsTextureSourceLoader::ResolveArraySize(const DdsHeader& header, const DdsHeaderDx10* dx10Header) noexcept
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

bool DdsTextureSourceLoader::IsBlockCompressed(DXGI_FORMAT format) noexcept
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

std::uint32_t DdsTextureSourceLoader::ComputeRowPitch(
	DXGI_FORMAT format,
	std::uint32_t width,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	if (IsBlockCompressed(format))
	{
		const std::uint32_t blockCountX = (std::max)(1u, (width + 3u) / 4u);
		return blockCountX * ResolveBlockSize(format, resolvedPath, outErrorMessage);
	}

	return (width * ResolveBitsPerPixel(format, resolvedPath, outErrorMessage) + 7u) / 8u;
}

std::uint32_t DdsTextureSourceLoader::ComputeSlicePitch(
	DXGI_FORMAT format,
	std::uint32_t width,
	std::uint32_t height,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	if (IsBlockCompressed(format))
	{
		const std::uint32_t blockCountY = (std::max)(1u, (height + 3u) / 4u);
		return ComputeRowPitch(format, width, resolvedPath, outErrorMessage) * blockCountY;
	}

	return ComputeRowPitch(format, width, resolvedPath, outErrorMessage) * height;
}

std::size_t DdsTextureSourceLoader::ResolvePixelDataOffset(const DdsHeader& header) noexcept
{
	return sizeof(kDdsMagic) + sizeof(DdsHeader) + (HasDx10Header(header) ? sizeof(DdsHeaderDx10) : 0u);
}

TextureLoadResult DdsTextureSourceLoader::BuildLoadResult(
	const std::vector<std::uint8_t>& fileBytes,
	const DdsHeader& header,
	const DdsHeaderDx10* dx10Header,
	DXGI_FORMAT dxgiFormat,
	const std::filesystem::path& resolvedPath,
	std::string& outErrorMessage)
{
	TextureLoadResult loadResult;
	loadResult.width = header.width;
	loadResult.height = header.height;
	loadResult.arraySize = ResolveArraySize(header, dx10Header);
	loadResult.dimension = IsCubemap(header, dx10Header)
	                            ? TextureResourceDimension::TextureCube
	                            : TextureResourceDimension::Texture2D;
	loadResult.dxgiFormat = dxgiFormat;
	loadResult.formatIntent = TextureFormatIntent::Unknown;
	loadResult.arraySlices.resize(loadResult.arraySize);
	for (TextureArraySliceData& arraySlice : loadResult.arraySlices)
	{
		arraySlice.mipLevels.reserve(ResolveMipCount(header));
	}

	std::size_t byteOffset = ResolvePixelDataOffset(header);
	for (std::uint32_t arraySliceIndex = 0; arraySliceIndex < loadResult.arraySize; ++arraySliceIndex)
	{
		std::uint32_t mipWidth = header.width;
		std::uint32_t mipHeight = header.height;

		for (std::uint32_t mipIndex = 0; mipIndex < ResolveMipCount(header); ++mipIndex)
		{
			TextureMipLevelData mipLevel;
			mipLevel.width = (std::max)(1u, mipWidth);
			mipLevel.height = (std::max)(1u, mipHeight);
			mipLevel.rowPitch = ComputeRowPitch(dxgiFormat, mipLevel.width, resolvedPath, outErrorMessage);
			if (!outErrorMessage.empty())
			{
				return {};
			}

			mipLevel.slicePitch = ComputeSlicePitch(dxgiFormat, mipLevel.width, mipLevel.height, resolvedPath, outErrorMessage);
			if (!outErrorMessage.empty())
			{
				return {};
			}

			if (byteOffset + mipLevel.slicePitch > fileBytes.size())
			{
				outErrorMessage = std::format(
				    "DDS texture '{}' ended before slice {} mip {} could be read",
				    resolvedPath.string(),
				    arraySliceIndex,
				    mipIndex);
				return {};
			}

			mipLevel.data.assign(
			    fileBytes.begin() + static_cast<std::ptrdiff_t>(byteOffset),
			    fileBytes.begin() + static_cast<std::ptrdiff_t>(byteOffset + mipLevel.slicePitch));
			loadResult.arraySlices[arraySliceIndex].mipLevels.push_back(std::move(mipLevel));

			byteOffset += loadResult.arraySlices[arraySliceIndex].mipLevels.back().slicePitch;
			mipWidth = (std::max)(1u, mipWidth >> 1u);
			mipHeight = (std::max)(1u, mipHeight >> 1u);
		}
	}

	outErrorMessage.clear();
	return loadResult;
}