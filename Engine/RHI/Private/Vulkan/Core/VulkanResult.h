#pragma once

#include "Vulkan/VulkanIncludes.h"

#include <string>
#include <string_view>

class VulkanResult final
{
  public:
	VulkanResult() = delete;

	static bool Succeeded(VkResult result) noexcept;
	static const char* ToString(VkResult result) noexcept;
	static std::string FormatFailure(std::string_view operation, VkResult result);
};