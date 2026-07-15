#pragma once

#include "RHI/Public/Formats/PixelFormat.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"
#include "RHI/Public/Resources/TextureTypes.h"

#include <cstdint>

struct RendererTexture final
{
	RhiOwnedResourceHandle Resource = {};
	RhiResourceViewHandle ShaderResourceView = {};
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t ArraySize = 0;
	TextureResourceDimension Dimension = TextureResourceDimension::Texture2D;
	PixelFormat Format = PixelFormat::Unknown;
	TextureFormatIntent FormatIntent = TextureFormatIntent::Unknown;
	std::uint16_t MipCount = 0;
	std::uint64_t EstimatedByteSize = 0;

	constexpr explicit operator bool() const noexcept
	{
		return Resource && ShaderResourceView;
	}
};
