#pragma once

#include "Diagnostics/RhiDiagnostics.h"
#include "Vulkan/VulkanIncludes.h"

#include <string_view>

struct VulkanDebugEventFunctions final
{
	PFN_vkCmdBeginDebugUtilsLabelEXT BeginLabel = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT EndLabel = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT InsertLabel = nullptr;
};

class VulkanDebugEvents final
{
  public:
	static bool SupportsScopes(VkCommandBuffer commandBuffer, const VulkanDebugEventFunctions& functions) noexcept;
	static void BeginScope(
	    VkCommandBuffer commandBuffer,
	    const VulkanDebugEventFunctions& functions,
	    std::string_view label,
	    RhiDiagnosticLabelColor color) noexcept;
	static void EndScope(VkCommandBuffer commandBuffer, const VulkanDebugEventFunctions& functions) noexcept;
	static void InsertMarker(
	    VkCommandBuffer commandBuffer,
	    const VulkanDebugEventFunctions& functions,
	    std::string_view label,
	    RhiDiagnosticLabelColor color) noexcept;

	VulkanDebugEvents() = delete;
	~VulkanDebugEvents() = delete;

  private:
	static VkDebugUtilsLabelEXT BuildLabel(const char* label, RhiDiagnosticLabelColor color) noexcept;
};