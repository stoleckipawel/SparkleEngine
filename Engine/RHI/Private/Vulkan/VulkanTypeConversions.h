#pragma once

#include "Formats/PixelFormat.h"
#include "Vulkan/VulkanIncludes.h"

class VulkanTypeConversions final
{
  public:
	VulkanTypeConversions() = delete;

	static VkFormat ToVkFormat(PixelFormat format) noexcept;
	static PixelFormat ToPixelFormat(VkFormat format) noexcept;
};