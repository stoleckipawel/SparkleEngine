#pragma once

#include <DirectXMath.h>

#include <cstdint>

enum class TextureColorSpace : std::uint8_t
{
	Linear = 0,
	Srgb = 1,
};

enum class TextureDimension : std::uint8_t
{
	Texture2D = 0,
	TextureCube = 1,
};

enum class TextureChannelMask : std::uint8_t
{
	Rgba = 0,
	Red = 1,
	Green = 2,
	Blue = 3,
	Alpha = 4,
};

enum class TextureAddressMode : std::uint32_t
{
	Repeat = 0,
	ClampToEdge = 1,
	MirroredRepeat = 2,
};

struct TextureCoordinateMapping final
{
	DirectX::XMFLOAT2 Offset = {0.0f, 0.0f};
	DirectX::XMFLOAT2 Scale = {1.0f, 1.0f};
	float Rotation = 0.0f;
	float Strength = 1.0f;
	TextureAddressMode AddressU = TextureAddressMode::Repeat;
	TextureAddressMode AddressV = TextureAddressMode::Repeat;
	std::uint32_t TexCoord = 0u;
};
