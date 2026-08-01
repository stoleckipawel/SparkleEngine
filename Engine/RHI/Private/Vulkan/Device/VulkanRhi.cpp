#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include "Vulkan/Commands/VulkanCommandQueue.h"
#include "Vulkan/Core/VulkanResult.h"

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

VulkanRhi::VulkanRhi() noexcept
{
	{
		CreateInstance();
	}
	{
		CreateDebugMessenger();
	}
	{
		SelectPhysicalDevice();
	}
	{
		CreateLogicalDevice();
	}
	LoadDeviceDebugFunctions();
	LoadRayTracingFunctions();
	BuildRayTracingCapabilities();
	NameBootstrapObjects();
	LogBootstrapSummary();
}

VulkanRhi::~VulkanRhi() noexcept
{
	if (m_device != VK_NULL_HANDLE)
	{
		for (std::unique_ptr<VulkanCommandQueue>& queue : m_queues)
		{
			queue.reset();
		}
		vkDestroyDevice(m_device, nullptr);
		m_device = VK_NULL_HANDLE;
	}

	m_debugLayer.Shutdown();

	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
}

void VulkanRhi::WaitForIdle() noexcept
{
	if (m_device != VK_NULL_HANDLE)
	{
		const VkResult result = vkDeviceWaitIdle(m_device);
		if (!VulkanResult::Succeeded(result))
		{
			SPDLOG_LOGGER_WARN(g_vulkanRhiLogger, "vkDeviceWaitIdle failed: {}", VulkanResult::ToString(result));
		}
	}
}

VkInstance VulkanRhi::GetInstance() const noexcept
{
	return m_instance;
}

VkPhysicalDevice VulkanRhi::GetPhysicalDevice() const noexcept
{
	return m_physicalDevice;
}

VkDevice VulkanRhi::GetDevice() const noexcept
{
	return m_device;
}

const VulkanAdapterInfo& VulkanRhi::GetAdapterInfo() const noexcept
{
	return m_adapterInfo;
}

const VulkanFeatureStatus& VulkanRhi::GetFeatureStatus() const noexcept
{
	return m_featureStatus;
}

const std::vector<std::string>& VulkanRhi::GetEnabledInstanceExtensions() const noexcept
{
	return m_enabledInstanceExtensions;
}

const std::vector<std::string>& VulkanRhi::GetEnabledDeviceExtensions() const noexcept
{
	return m_enabledDeviceExtensions;
}

bool VulkanRhi::IsValidationEnabled() const noexcept
{
	return m_validationEnabled;
}
