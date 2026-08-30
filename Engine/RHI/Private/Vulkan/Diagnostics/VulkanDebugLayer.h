#pragma once

#include "Vulkan/VulkanIncludes.h"

class VulkanDebugLayer final
{
public:
	VulkanDebugLayer() noexcept = default;
	~VulkanDebugLayer() noexcept;

	VulkanDebugLayer(const VulkanDebugLayer&) = delete;
	VulkanDebugLayer& operator=(const VulkanDebugLayer&) = delete;
	VulkanDebugLayer(VulkanDebugLayer&&) = delete;
	VulkanDebugLayer& operator=(VulkanDebugLayer&&) = delete;

	void Initialize(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData) noexcept;
	void Shutdown() noexcept;

	VkDebugUtilsMessengerEXT GetMessenger() const noexcept { return m_messenger; }

private:
	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
	PFN_vkDestroyDebugUtilsMessengerEXT m_destroyMessenger = nullptr;
};
