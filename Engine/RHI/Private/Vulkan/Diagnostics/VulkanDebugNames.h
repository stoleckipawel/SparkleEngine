#pragma once

#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <string_view>

class VulkanDebugNames final
{
  public:
	VulkanDebugNames() = delete;

	static bool SetObjectName(
	    PFN_vkSetDebugUtilsObjectNameEXT setObjectName,
	    VkDevice device,
	    VkObjectType objectType,
	    std::uint64_t objectHandle,
	    std::string_view name) noexcept;
	static const char* ObjectTypeToString(VkObjectType objectType) noexcept;
};