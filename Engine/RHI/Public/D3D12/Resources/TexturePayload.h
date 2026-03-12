// ============================================================================
// TexturePayload.h
// ----------------------------------------------------------------------------
// Runtime texture payload used to upload textures created outside the legacy
// file-loader path. Designed for future stb/mip-generation integration.
// ============================================================================

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

struct TextureMipLevelData
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	std::uint32_t rowPitch = 0;
	std::uint32_t slicePitch = 0;
	std::vector<std::uint8_t> data;
};

struct TexturePayload
{
	std::uint32_t width = 1;
	std::uint32_t height = 1;
	DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
	TextureFormatIntent formatIntent = TextureFormatIntent::Unknown;
	std::vector<TextureMipLevelData> mipLevels;

	std::uint16_t GetMipCount() const noexcept
	{
		return static_cast<std::uint16_t>(mipLevels.size());
	}

	bool IsValid() const noexcept
	{
		if (width == 0 || height == 0 || dxgiFormat == DXGI_FORMAT_UNKNOWN || mipLevels.empty())
		{
			return false;
		}

		for (const auto& mip : mipLevels)
		{
			if (mip.width == 0 || mip.height == 0 || mip.rowPitch == 0 || mip.slicePitch == 0 || mip.data.empty())
			{
				return false;
			}
		}

		return true;
	}
};