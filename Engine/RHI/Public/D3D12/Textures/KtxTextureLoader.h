#pragma once

#include "../../RHIAPI.h"
#include "TextureLoadResult.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include <dxgiformat.h>

class SPARKLE_RHI_API KtxTextureLoader final
{
  public:
	static TextureLoadResult Load(const std::filesystem::path& fileName);
	static bool SupportsExtension(std::wstring_view extension) noexcept;

	KtxTextureLoader() = delete;
	~KtxTextureLoader() = delete;

  private:
	static constexpr std::uint32_t kVkFormatR8G8B8A8Unorm = 37;
	static constexpr std::uint32_t kVkFormatR8G8B8A8Srgb = 43;
	static constexpr std::uint32_t kVkFormatB8G8R8A8Unorm = 44;
	static constexpr std::uint32_t kVkFormatB8G8R8A8Srgb = 50;
	static constexpr std::uint32_t kVkFormatBc1RgbaUnorm = 133;
	static constexpr std::uint32_t kVkFormatBc1RgbaSrgb = 134;
	static constexpr std::uint32_t kVkFormatBc2Unorm = 135;
	static constexpr std::uint32_t kVkFormatBc2Srgb = 136;
	static constexpr std::uint32_t kVkFormatBc3Unorm = 137;
	static constexpr std::uint32_t kVkFormatBc3Srgb = 138;
	static constexpr std::uint32_t kVkFormatBc4Unorm = 139;
	static constexpr std::uint32_t kVkFormatBc4Snorm = 140;
	static constexpr std::uint32_t kVkFormatBc5Unorm = 141;
	static constexpr std::uint32_t kVkFormatBc5Snorm = 142;

	static bool ResolveDxgiFormat(
	    std::uint32_t vkFormat,
	    DXGI_FORMAT& outDxgiFormat,
	    TextureFormatIntent& outFormatIntent,
	    std::string& outErrorMessage);
	static bool ValidateTextureShape(
	    std::uint32_t numDimensions,
	    bool isArray,
	    bool isCubemap,
	    std::uint32_t numLayers,
	    std::uint32_t numFaces,
	    const std::filesystem::path& resolvedPath,
	    std::string& outErrorMessage);
	static bool ValidateMipPayloadRange(
	    std::size_t byteOffset,
	    std::size_t byteCount,
	    std::size_t dataSize,
	    const std::filesystem::path& resolvedPath,
	    std::string& outErrorMessage);
	static std::uint32_t ResolveRowPitch(DXGI_FORMAT dxgiFormat, std::uint32_t width) noexcept;
	static std::uint32_t ResolveSlicePitch(DXGI_FORMAT dxgiFormat, std::uint32_t width, std::uint32_t height) noexcept;
	static bool IsBlockCompressed(DXGI_FORMAT dxgiFormat) noexcept;
	static std::uint32_t ResolveBlockSize(DXGI_FORMAT dxgiFormat) noexcept;
	static std::uint32_t ResolveBytesPerPixel(DXGI_FORMAT dxgiFormat) noexcept;
};