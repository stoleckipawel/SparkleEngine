#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanDebugEvents.h"

#include <string>

bool VulkanDebugEvents::SupportsScopes(VkCommandBuffer commandBuffer, const VulkanDebugEventFunctions& functions) noexcept
{
	return commandBuffer != VK_NULL_HANDLE && functions.BeginLabel != nullptr && functions.EndLabel != nullptr;
}

void VulkanDebugEvents::BeginScope(
    VkCommandBuffer commandBuffer,
    const VulkanDebugEventFunctions& functions,
    std::string_view label,
    RhiDiagnosticLabelColor color) noexcept
{
	if (!SupportsScopes(commandBuffer, functions))
	{
		return;
	}

	const std::string ownedLabel(label);
	const VkDebugUtilsLabelEXT nativeLabel = BuildLabel(ownedLabel.c_str(), color);
	functions.BeginLabel(commandBuffer, &nativeLabel);
}

void VulkanDebugEvents::EndScope(VkCommandBuffer commandBuffer, const VulkanDebugEventFunctions& functions) noexcept
{
	if (commandBuffer != VK_NULL_HANDLE && functions.EndLabel != nullptr)
	{
		functions.EndLabel(commandBuffer);
	}
}

void VulkanDebugEvents::InsertMarker(
    VkCommandBuffer commandBuffer,
    const VulkanDebugEventFunctions& functions,
    std::string_view label,
    RhiDiagnosticLabelColor color) noexcept
{
	if (commandBuffer == VK_NULL_HANDLE || functions.InsertLabel == nullptr)
	{
		return;
	}

	const std::string ownedLabel(label);
	const VkDebugUtilsLabelEXT nativeLabel = BuildLabel(ownedLabel.c_str(), color);
	functions.InsertLabel(commandBuffer, &nativeLabel);
}

VkDebugUtilsLabelEXT VulkanDebugEvents::BuildLabel(const char* label, RhiDiagnosticLabelColor color) noexcept
{
	return VkDebugUtilsLabelEXT{
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
	    .pNext = nullptr,
	    .pLabelName = label,
	    .color = {
	        static_cast<float>(color.Red) / 255.0f,
	        static_cast<float>(color.Green) / 255.0f,
	        static_cast<float>(color.Blue) / 255.0f,
	        static_cast<float>(color.Alpha) / 255.0f}};
}