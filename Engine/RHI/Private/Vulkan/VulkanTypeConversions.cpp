#include "Vulkan/VulkanPCH.h"

#include "Vulkan/VulkanTypeConversions.h"

VkFormat VulkanTypeConversions::ToVkFormat(PixelFormat format) noexcept
{
	switch (format)
	{
		case PixelFormat::R8G8B8A8_UNorm:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::B8G8R8A8_UNorm:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case PixelFormat::R16G16B16A16_Float:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case PixelFormat::D24_UNorm_S8_UInt:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case PixelFormat::R32_Float:
			return VK_FORMAT_R32_SFLOAT;
		case PixelFormat::Unknown:
		default:
			return VK_FORMAT_UNDEFINED;
	}
}

PixelFormat VulkanTypeConversions::ToPixelFormat(VkFormat format) noexcept
{
	switch (format)
	{
		case VK_FORMAT_R8G8B8A8_UNORM:
			return PixelFormat::R8G8B8A8_UNorm;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return PixelFormat::B8G8R8A8_UNorm;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return PixelFormat::R16G16B16A16_Float;
		case VK_FORMAT_D24_UNORM_S8_UINT:
			return PixelFormat::D24_UNorm_S8_UInt;
		case VK_FORMAT_R32_SFLOAT:
			return PixelFormat::R32_Float;
		default:
			return PixelFormat::Unknown;
	}
}