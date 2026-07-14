#pragma once

#include <DirectXMath.h>

#include "RHI/Public/Formats/PixelFormat.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"
#include "RHI/Public/Resources/TextureTypes.h"

struct RenderSkyData final
{
	RhiOwnedResourceHandle textureResource = {};
	RhiResourceViewHandle textureView = {};
	std::uint32_t textureWidth = 0;
	std::uint32_t textureHeight = 0;
	std::uint32_t textureArraySize = 0;
	TextureResourceDimension textureDimension = TextureResourceDimension::Texture2D;
	PixelFormat textureFormat = PixelFormat::Unknown;
	TextureFormatIntent textureFormatIntent = TextureFormatIntent::Unknown;
	std::uint16_t textureMipCount = 0;
	std::uint64_t textureEstimatedByteSize = 0;
	DirectX::XMFLOAT3 color{1.0f, 1.0f, 1.0f};
	float intensity = 1.0f;
	bool enabled = true;

	bool HasTexture() const noexcept { return textureResource && textureView; }
};
