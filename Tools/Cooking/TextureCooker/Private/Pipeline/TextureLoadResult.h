#pragma once

#include "Resources/TextureTypes.h"

#include <cstdint>
#include <vector>

#include <dxgi1_6.h>

struct TextureMipLevelData
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	std::uint32_t rowPitch = 0;
	std::uint32_t slicePitch = 0;
	std::vector<std::uint8_t> data;
};

using TextureArraySliceData = std::vector<TextureMipLevelData>;

struct TextureLoadResult
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	std::uint32_t arraySize = 1;
	TextureResourceDimension dimension = TextureResourceDimension::Texture2D;
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
	TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
	std::vector<TextureArraySliceData> arraySlices;

	std::uint16_t GetMipCount() const noexcept { return static_cast<std::uint16_t>(arraySlices.front().size()); }

	std::uint16_t GetArraySize() const noexcept { return static_cast<std::uint16_t>(arraySize); }

	std::uint32_t GetSubresourceCount() const noexcept { return static_cast<std::uint32_t>(GetMipCount()) * arraySize; }

	bool IsCube() const noexcept { return dimension == TextureResourceDimension::TextureCube; }
};
