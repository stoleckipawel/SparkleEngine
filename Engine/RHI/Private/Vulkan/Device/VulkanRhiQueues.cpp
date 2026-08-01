#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Device/VulkanRhi.h"

#include "Vulkan/Commands/VulkanCommandQueue.h"

static const auto g_vulkanRhiLogger = Logging::GetOrCreateLogger("RHI.Vulkan");

VkQueue VulkanRhi::GetGraphicsQueue() const noexcept
{
	return GetQueue(ERhiQueueType::Graphics);
}

VkQueue VulkanRhi::GetQueue(ERhiQueueType queueType) const noexcept
{
	const std::unique_ptr<VulkanCommandQueue>& queue = m_queues[RhiQueueTypeToIndex(queueType)];
	return queue != nullptr ? queue->GetNativeQueue() : VK_NULL_HANDLE;
}

VulkanCommandQueue& VulkanRhi::GetCommandQueue(ERhiQueueType queueType) noexcept
{
	return *m_queues[RhiQueueTypeToIndex(queueType)];
}

const VulkanCommandQueue& VulkanRhi::GetCommandQueue(ERhiQueueType queueType) const noexcept
{
	return *m_queues[RhiQueueTypeToIndex(queueType)];
}

std::uint32_t VulkanRhi::GetGraphicsQueueFamilyIndex() const noexcept
{
	return GetQueueFamilyIndex(ERhiQueueType::Graphics);
}

std::uint32_t VulkanRhi::GetQueueFamilyIndex(ERhiQueueType queueType) const noexcept
{
	return m_queueTopology.Get(queueType).FamilyIndex;
}

std::uint32_t VulkanRhi::GetQueueIndex(ERhiQueueType queueType) const noexcept
{
	return m_queueTopology.Get(queueType).QueueIndex;
}

bool VulkanRhi::HasIndependentQueue(ERhiQueueType queueType) const noexcept
{
	return m_queueTopology.HasIndependentQueue(queueType);
}

void VulkanRhi::ConfigureResourceQueueSharing(VkBufferCreateInfo& createInfo) const noexcept
{
	const std::span<const std::uint32_t> familyIndices = m_queueTopology.GetFamilyIndices();
	if (familyIndices.size() <= 1)
	{
		return;
	}
	createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	createInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(familyIndices.size());
	createInfo.pQueueFamilyIndices = familyIndices.data();
}

void VulkanRhi::ConfigureResourceQueueSharing(VkImageCreateInfo& createInfo) const noexcept
{
	const std::span<const std::uint32_t> familyIndices = m_queueTopology.GetFamilyIndices();
	if (familyIndices.size() <= 1)
	{
		return;
	}
	createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	createInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(familyIndices.size());
	createInfo.pQueueFamilyIndices = familyIndices.data();
}

void VulkanRhi::ConfigureResourceQueueSharing(VkSwapchainCreateInfoKHR& createInfo) const noexcept
{
	const std::span<const std::uint32_t> familyIndices = m_queueTopology.GetFamilyIndices();
	if (familyIndices.size() <= 1)
	{
		return;
	}
	createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	createInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(familyIndices.size());
	createInfo.pQueueFamilyIndices = familyIndices.data();
}
