#pragma once

#include "SourceLoading/TextureSourceLoaderBackend.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <dxgiformat.h>

class DdsTextureSourceLoader final : public TextureSourceLoaderBackend
{
public:
	bool SupportsFormat(TextureSourceFormat format) const noexcept override;
	TextureLoadResult Load(const std::filesystem::path& sourcePath) const override;

private:
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

	static DdsHeader ReadHeader(const std::vector<std::uint8_t>& fileBytes);
	static bool HasDx10Header(const DdsHeader& header) noexcept;
	static DdsHeaderDx10 ReadDx10Header(const std::vector<std::uint8_t>& fileBytes);
	static void ValidateHeader(const DdsHeader& header, const DdsHeaderDx10* dx10Header, const std::filesystem::path& resolvedPath);
	static DXGI_FORMAT ResolveDxgiFormat(
	    const DdsHeader& header,
	    const DdsHeaderDx10* dx10Header,
	    const std::filesystem::path& resolvedPath);
	static std::uint32_t ResolveBitsPerPixel(DXGI_FORMAT format, const std::filesystem::path& resolvedPath);
	static std::uint32_t ResolveBlockSize(DXGI_FORMAT format, const std::filesystem::path& resolvedPath);
	static std::uint32_t ResolveMipCount(const DdsHeader& header) noexcept;
	static bool IsCubemap(const DdsHeader& header, const DdsHeaderDx10* dx10Header) noexcept;
	static std::uint32_t ResolveArraySize(const DdsHeader& header, const DdsHeaderDx10* dx10Header) noexcept;
	static bool IsBlockCompressed(DXGI_FORMAT format) noexcept;
	static std::uint32_t ComputeRowPitch(DXGI_FORMAT format, std::uint32_t width, const std::filesystem::path& resolvedPath);
	static std::uint32_t ComputeSlicePitch(
	    DXGI_FORMAT format,
	    std::uint32_t width,
	    std::uint32_t height,
	    const std::filesystem::path& resolvedPath);
	static std::size_t ResolvePixelDataOffset(const DdsHeader& header) noexcept;
	static TextureLoadResult BuildLoadResult(
	    const std::vector<std::uint8_t>& fileBytes,
	    const DdsHeader& header,
	    const DdsHeaderDx10* dx10Header,
	    DXGI_FORMAT dxgiFormat,
	    const std::filesystem::path& resolvedPath);
};
