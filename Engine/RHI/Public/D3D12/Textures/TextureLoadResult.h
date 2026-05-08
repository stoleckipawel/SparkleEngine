#pragma once

#include <cstdint>
#include <vector>

#include <dxgi1_6.h>

enum class TextureFormatIntent : std::uint8_t
{
	Unknown,
	ColorSrgb,
	DataLinear
};

enum class TextureResourceDimension : std::uint8_t
{
	Texture2D = 0,
	TextureCube = 1,
};

struct TextureMipLevelData
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	std::uint32_t rowPitch = 0;
	std::uint32_t slicePitch = 0;
	std::vector<std::uint8_t> data;
};

struct TextureArraySliceData
{
	std::vector<TextureMipLevelData> mipLevels;
};

struct TextureLoadResult
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	std::uint32_t arraySize = 1;
	TextureResourceDimension dimension = TextureResourceDimension::Texture2D;
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
	TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
	std::vector<TextureArraySliceData> arraySlices;

	std::uint16_t GetMipCount() const noexcept
	{
		return arraySlices.empty() ? 0 : static_cast<std::uint16_t>(arraySlices.front().mipLevels.size());
	}

	std::uint16_t GetArraySize() const noexcept { return static_cast<std::uint16_t>(arraySize); }

	std::uint32_t GetSubresourceCount() const noexcept
	{
		return static_cast<std::uint32_t>(GetMipCount()) * arraySize;
	}

	bool IsCube() const noexcept { return dimension == TextureResourceDimension::TextureCube; }

	bool IsValid() const noexcept
	{
		if (width == 0 || height == 0 || arraySize == 0 || dxgiFormat == DXGI_FORMAT_UNKNOWN || arraySlices.empty())
		{
			return false;
		}

		if (arraySlices.size() != arraySize)
		{
			return false;
		}

		const std::size_t expectedMipCount = arraySlices.front().mipLevels.size();
		if (expectedMipCount == 0)
		{
			return false;
		}

		if (IsCube() && arraySize != 6)
		{
			return false;
		}

		for (const TextureArraySliceData& arraySlice : arraySlices)
		{
			if (arraySlice.mipLevels.size() != expectedMipCount)
			{
				return false;
			}

			for (const auto& mip : arraySlice.mipLevels)
			{
				if (mip.width == 0 || mip.height == 0 || mip.rowPitch == 0 || mip.slicePitch == 0 || mip.data.empty())
				{
					return false;
				}
			}
		}

		return true;
	}
};