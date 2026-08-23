#include "PCH.h"

#include "Formats/PixelFormat.h"

const char* PixelFormatName(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R8G8B8A8_UNorm:
			return "R8G8B8A8_UNorm";
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
			return "R8G8B8A8_UNorm_Srgb";
		case PixelFormat::B8G8R8A8_UNorm:
			return "B8G8R8A8_UNorm";
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return "B8G8R8A8_UNorm_Srgb";
		case PixelFormat::R16G16B16A16_Float:
			return "R16G16B16A16_Float";
		case PixelFormat::R16G16_Float:
			return "R16G16_Float";
		case PixelFormat::R32G32B32A32_Float:
			return "R32G32B32A32_Float";
		case PixelFormat::D32_Float:
			return "D32_Float";
		case PixelFormat::R32_Float:
			return "R32_Float";
		case PixelFormat::D24_UNorm_S8_UInt:
			return "D24_UNorm_S8_UInt";
		case PixelFormat::BC1_UNorm:
			return "BC1_UNorm";
		case PixelFormat::BC1_UNorm_Srgb:
			return "BC1_UNorm_Srgb";
		case PixelFormat::BC2_UNorm:
			return "BC2_UNorm";
		case PixelFormat::BC2_UNorm_Srgb:
			return "BC2_UNorm_Srgb";
		case PixelFormat::BC3_UNorm:
			return "BC3_UNorm";
		case PixelFormat::BC3_UNorm_Srgb:
			return "BC3_UNorm_Srgb";
		case PixelFormat::BC4_UNorm:
			return "BC4_UNorm";
		case PixelFormat::BC4_SNorm:
			return "BC4_SNorm";
		case PixelFormat::BC5_UNorm:
			return "BC5_UNorm";
		case PixelFormat::BC5_SNorm:
			return "BC5_SNorm";
		case PixelFormat::BC6H_UF16:
			return "BC6H_UF16";
		case PixelFormat::BC7_UNorm:
			return "BC7_UNorm";
		case PixelFormat::BC7_UNorm_Srgb:
			return "BC7_UNorm_Srgb";
		case PixelFormat::Unknown:
		default:
			return "Unknown";
	}
}

bool IsColorAttachmentPixelFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R32G32B32A32_Float:
		case PixelFormat::R16G16B16A16_Float:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
		case PixelFormat::R16G16_Float:
		case PixelFormat::R32_Float:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return true;
		default:
			return false;
	}
}

bool IsDepthStencilPixelFormat(PixelFormat format) noexcept
{
	return format == PixelFormat::D32_Float || format == PixelFormat::D24_UNorm_S8_UInt;
}

bool PixelFormatHasStencilAspect(PixelFormat format) noexcept
{
	return format == PixelFormat::D24_UNorm_S8_UInt;
}

bool IsSrgbPixelFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
		case PixelFormat::BC1_UNorm_Srgb:
		case PixelFormat::BC2_UNorm_Srgb:
		case PixelFormat::BC3_UNorm_Srgb:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
		case PixelFormat::BC7_UNorm_Srgb:
			return true;
		default:
			return false;
	}
}

PixelFormat PixelFormatToLinear(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
			return PixelFormat::R8G8B8A8_UNorm;
		case PixelFormat::BC1_UNorm_Srgb:
			return PixelFormat::BC1_UNorm;
		case PixelFormat::BC2_UNorm_Srgb:
			return PixelFormat::BC2_UNorm;
		case PixelFormat::BC3_UNorm_Srgb:
			return PixelFormat::BC3_UNorm;
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return PixelFormat::B8G8R8A8_UNorm;
		case PixelFormat::BC7_UNorm_Srgb:
			return PixelFormat::BC7_UNorm;
		default:
			return format;
	}
}

std::uint32_t PixelFormatBytesPerTexel(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R32G32B32A32_Float:
			return 16u;
		case PixelFormat::R16G16B16A16_Float:
			return 8u;
		case PixelFormat::R16G16_Float:
		case PixelFormat::D32_Float:
		case PixelFormat::R32_Float:
		case PixelFormat::D24_UNorm_S8_UInt:
		case PixelFormat::R8G8B8A8_UNorm:
		case PixelFormat::R8G8B8A8_UNorm_Srgb:
		case PixelFormat::B8G8R8A8_UNorm:
		case PixelFormat::B8G8R8A8_UNorm_Srgb:
			return 4u;
		default:
			return 0u;
	}
}

PixelFormat PixelFormatFromSerializedTextureFormat(std::uint32_t value) noexcept
{
	switch (value)
	{
		case 2u:
			return PixelFormat::R32G32B32A32_Float;
		case 10u:
			return PixelFormat::R16G16B16A16_Float;
		case 28u:
			return PixelFormat::R8G8B8A8_UNorm;
		case 29u:
			return PixelFormat::R8G8B8A8_UNorm_Srgb;
		case 34u:
			return PixelFormat::R16G16_Float;
		case 40u:
			return PixelFormat::D32_Float;
		case 41u:
			return PixelFormat::R32_Float;
		case 45u:
			return PixelFormat::D24_UNorm_S8_UInt;
		case 71u:
			return PixelFormat::BC1_UNorm;
		case 72u:
			return PixelFormat::BC1_UNorm_Srgb;
		case 74u:
			return PixelFormat::BC2_UNorm;
		case 75u:
			return PixelFormat::BC2_UNorm_Srgb;
		case 77u:
			return PixelFormat::BC3_UNorm;
		case 78u:
			return PixelFormat::BC3_UNorm_Srgb;
		case 80u:
			return PixelFormat::BC4_UNorm;
		case 81u:
			return PixelFormat::BC4_SNorm;
		case 83u:
			return PixelFormat::BC5_UNorm;
		case 84u:
			return PixelFormat::BC5_SNorm;
		case 87u:
			return PixelFormat::B8G8R8A8_UNorm;
		case 91u:
			return PixelFormat::B8G8R8A8_UNorm_Srgb;
		case 95u:
			return PixelFormat::BC6H_UF16;
		case 98u:
			return PixelFormat::BC7_UNorm;
		case 99u:
			return PixelFormat::BC7_UNorm_Srgb;
		default:
			return PixelFormat::Unknown;
	}
}

std::uint32_t PixelFormatToSerializedTextureFormat(PixelFormat format) noexcept
{
	return static_cast<std::uint32_t>(format);
}
