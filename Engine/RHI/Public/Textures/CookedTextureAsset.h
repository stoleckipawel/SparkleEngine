#pragma once

#include "../Resources/TextureTypes.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>
#include <type_traits>

constexpr std::uint32_t MakeCookedTextureAssetMagic(char a, char b, char c, char d) noexcept
{
	return static_cast<std::uint32_t>(static_cast<std::uint8_t>(a)) | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(b)) << 8u)
	    | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(c)) << 16u)
	    | (static_cast<std::uint32_t>(static_cast<std::uint8_t>(d)) << 24u);
}

inline constexpr std::string_view kCookedTextureAssetExtension = ".stex";
inline constexpr std::wstring_view kCookedTextureAssetExtensionWide = L".stex";
inline constexpr std::uint32_t kCookedTextureAssetMagic = MakeCookedTextureAssetMagic('S', 'T', 'E', 'X');

constexpr std::uint32_t PackCookedTextureLayout(TextureResourceDimension dimension, std::uint16_t arraySize) noexcept
{
	return static_cast<std::uint32_t>(arraySize) | (static_cast<std::uint32_t>(dimension) << 16u);
}

constexpr std::uint16_t UnpackCookedTextureArraySize(std::uint32_t packedLayout) noexcept
{
	const std::uint16_t arraySize = static_cast<std::uint16_t>(packedLayout & 0xffffu);
	return arraySize == 0 ? 1 : arraySize;
}

constexpr TextureResourceDimension UnpackCookedTextureDimension(std::uint32_t packedLayout) noexcept
{
	return static_cast<TextureResourceDimension>((packedLayout >> 16u) & 0xffu);
}

struct SPARKLE_RHI_API CookedTextureAssetHeader
{
	std::uint32_t magic = kCookedTextureAssetMagic;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	std::uint32_t format = 0;
	std::uint32_t formatIntent = static_cast<std::uint32_t>(TextureFormatIntent::Unknown);
	std::uint32_t mipCount = 0;
	std::uint32_t packedLayout = PackCookedTextureLayout(TextureResourceDimension::Texture2D, 1);

	constexpr bool HasExpectedMagic() const noexcept { return magic == kCookedTextureAssetMagic; }

	constexpr std::uint16_t GetArraySize() const noexcept { return UnpackCookedTextureArraySize(packedLayout); }
	constexpr TextureResourceDimension GetDimension() const noexcept { return UnpackCookedTextureDimension(packedLayout); }
};

struct SPARKLE_RHI_API CookedTextureMipHeader
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	std::uint32_t rowPitch = 0;
	std::uint32_t slicePitch = 0;
	std::uint32_t dataSize = 0;
	std::uint32_t reserved = 0;
};

static_assert(std::is_trivially_copyable_v<CookedTextureAssetHeader>, "CookedTextureAssetHeader must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<CookedTextureMipHeader>, "CookedTextureMipHeader must stay trivially copyable.");
