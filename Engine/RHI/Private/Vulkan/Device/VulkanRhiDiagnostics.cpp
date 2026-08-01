#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include <utility>

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

bool VulkanRhi::TryPopDiagnosticMessage(RhiDiagnosticMessage& outMessage) noexcept
{
	return m_diagnosticMessageQueue.TryPop(outMessage);
}

void VulkanRhi::ClearDiagnosticMessages() noexcept
{
	m_diagnosticMessageQueue.Clear();
}

PFN_vkSetDebugUtilsObjectNameEXT VulkanRhi::GetSetDebugUtilsObjectName() const noexcept
{
	return m_setDebugUtilsObjectName;
}

PFN_vkCmdBeginDebugUtilsLabelEXT VulkanRhi::GetCmdBeginDebugUtilsLabel() const noexcept
{
	return m_cmdBeginDebugUtilsLabel;
}

PFN_vkCmdEndDebugUtilsLabelEXT VulkanRhi::GetCmdEndDebugUtilsLabel() const noexcept
{
	return m_cmdEndDebugUtilsLabel;
}

PFN_vkCmdInsertDebugUtilsLabelEXT VulkanRhi::GetCmdInsertDebugUtilsLabel() const noexcept
{
	return m_cmdInsertDebugUtilsLabel;
}

void VulkanRhi::LoadDeviceDebugFunctions() noexcept
{
	if (m_device == VK_NULL_HANDLE)
	{
		return;
	}

	m_setDebugUtilsObjectName =
	    reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(m_device, "vkSetDebugUtilsObjectNameEXT"));
	m_cmdBeginDebugUtilsLabel =
	    reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_device, "vkCmdBeginDebugUtilsLabelEXT"));
	m_cmdEndDebugUtilsLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_device, "vkCmdEndDebugUtilsLabelEXT"));
	m_cmdInsertDebugUtilsLabel =
	    reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetDeviceProcAddr(m_device, "vkCmdInsertDebugUtilsLabelEXT"));
}

void VulkanRhi::PushDiagnosticMessage(
    ERhiDiagnosticMessageSeverity severity,
    ERhiDiagnosticMessageCategory category,
    std::string text) noexcept
{
	m_diagnosticMessageQueue.Push(RhiDiagnosticMessage{.Severity = severity, .Category = category, .Text = std::move(text)});
}

VkBool32 VKAPI_PTR VulkanRhi::DebugUtilsCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData) noexcept
{
	auto* rhi = static_cast<VulkanRhi*>(userData);
	if (rhi == nullptr || callbackData == nullptr)
	{
		return VK_FALSE;
	}

	ERhiDiagnosticMessageSeverity sparkleSeverity = ERhiDiagnosticMessageSeverity::Info;
	if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
	{
		sparkleSeverity = ERhiDiagnosticMessageSeverity::Error;
	}
	else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
	{
		sparkleSeverity = ERhiDiagnosticMessageSeverity::Warning;
	}
	else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0)
	{
		sparkleSeverity = ERhiDiagnosticMessageSeverity::Verbose;
	}

	ERhiDiagnosticMessageCategory sparkleCategory = ERhiDiagnosticMessageCategory::General;
	if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
	{
		sparkleCategory = ERhiDiagnosticMessageCategory::Validation;
	}
	else if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0)
	{
		sparkleCategory = ERhiDiagnosticMessageCategory::Performance;
	}

	const char* const message = callbackData->pMessage != nullptr ? callbackData->pMessage : "";
	rhi->PushDiagnosticMessage(sparkleSeverity, sparkleCategory, message);
	return VK_FALSE;
}
