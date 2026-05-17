#pragma once

#include "../Formats/PixelFormat.h"
#include "TextureTypes.h"

#include <cstdint>
#include <vector>

struct RhiTextureMipUploadData
{
	std::uint32_t Width = 1;
	std::uint32_t Height = 1;
	std::uint32_t RowPitch = 0;
	std::uint32_t SlicePitch = 0;
	std::vector<std::uint8_t> Data;
};

struct RhiTextureArraySliceUploadData
{
	std::vector<RhiTextureMipUploadData> MipLevels;
};

struct RhiTextureUploadDesc
{
	std::uint32_t Width = 1;
	std::uint32_t Height = 1;
	std::uint32_t ArraySize = 1;
	TextureResourceDimension Dimension = TextureResourceDimension::Texture2D;
	PixelFormat Format = PixelFormat::Unknown;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;
	std::vector<RhiTextureArraySliceUploadData> ArraySlices;

	std::uint16_t GetMipCount() const noexcept
	{
		return ArraySlices.empty() ? 0 : static_cast<std::uint16_t>(ArraySlices.front().MipLevels.size());
	}

	std::uint16_t GetArraySize() const noexcept { return static_cast<std::uint16_t>(ArraySize); }

	std::uint32_t GetSubresourceCount() const noexcept { return static_cast<std::uint32_t>(GetMipCount()) * ArraySize; }

	bool IsCube() const noexcept { return Dimension == TextureResourceDimension::TextureCube; }

	bool IsValid() const noexcept
	{
		if (Width == 0 || Height == 0 || ArraySize == 0 || Format == PixelFormat::Unknown || ArraySlices.empty())
		{
			return false;
		}

		if (ArraySlices.size() != ArraySize)
		{
			return false;
		}

		const std::size_t expectedMipCount = ArraySlices.front().MipLevels.size();
		if (expectedMipCount == 0)
		{
			return false;
		}

		if (IsCube() && ArraySize != 6)
		{
			return false;
		}

		for (const RhiTextureArraySliceUploadData& arraySlice : ArraySlices)
		{
			if (arraySlice.MipLevels.size() != expectedMipCount)
			{
				return false;
			}

			for (const RhiTextureMipUploadData& mip : arraySlice.MipLevels)
			{
				if (mip.Width == 0 || mip.Height == 0 || mip.RowPitch == 0 || mip.SlicePitch == 0 || mip.Data.empty())
				{
					return false;
				}
			}
		}

		return true;
	}
};