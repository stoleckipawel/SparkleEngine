#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Diagnostics/VulkanDebugLayer.h"

#include "Vulkan/Core/VulkanResult.h"

static const auto g_vulkanDebugLayerLogger = Logging::GetOrCreateLogger("RHI.Vulkan.DebugLayer");

VulkanDebugLayer::~VulkanDebugLayer() noexcept
{
	Shutdown();
}

void VulkanDebugLayer::Initialize(VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData) noexcept
{
	Shutdown();
	m_instance = instance;
	if (m_instance == VK_NULL_HANDLE || callback == nullptr)
	{
		return;
	}

	auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
	    vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
	m_destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
	    vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (createMessenger == nullptr || m_destroyMessenger == nullptr)
	{
		return;
	}

#if ENGINE_GPU_VALIDATION
	const VkDebugUtilsMessengerCreateInfoEXT createInfo{
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	    .pNext = nullptr,
	    .flags = 0,
	    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	    .pfnUserCallback = callback,
	    .pUserData = userData};

	const VkResult result = createMessenger(m_instance, &createInfo, nullptr, &m_messenger);
	if (!VulkanResult::Succeeded(result))
	{
		SPDLOG_LOGGER_WARN(g_vulkanDebugLayerLogger, "vkCreateDebugUtilsMessengerEXT failed: {}", VulkanResult::ToString(result));
	}
#endif
}

void VulkanDebugLayer::Shutdown() noexcept
{
	if (m_instance != VK_NULL_HANDLE && m_messenger != VK_NULL_HANDLE && m_destroyMessenger != nullptr)
	{
		m_destroyMessenger(m_instance, m_messenger, nullptr);
	}
	m_messenger = VK_NULL_HANDLE;
	m_destroyMessenger = nullptr;
	m_instance = VK_NULL_HANDLE;
}