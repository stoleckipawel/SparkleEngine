#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanRecordingUploadPage.h"

#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"

#include <algorithm>
#include <cstring>
#include <utility>

VulkanRecordingUploadPage::VulkanRecordingUploadPage(
    VulkanRhi& rhi,
    VulkanGpuMemoryAllocator& memoryAllocator,
    std::uint64_t capacityInBytes) noexcept :
    m_memoryAllocator(&memoryAllocator),
    m_capacityInBytes(capacityInBytes)
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(rhi.GetPhysicalDevice(), &properties);
	m_alignment = std::max<VkDeviceSize>(properties.limits.minUniformBufferOffsetAlignment, 1);

	const VkBufferCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = capacityInBytes,
	    .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
	m_buffer = memoryAllocator.CreateBuffer(
	    createInfo,
	    RhiMemoryCategory::ConstantBuffer,
	    RhiMemoryResidencyClass::HostUpload,
	    L"Vulkan Recording Upload Page");
	if (m_buffer != nullptr)
	{
		m_mappedData = static_cast<std::byte*>(memoryAllocator.MapUploadPage(*m_buffer));
	}
}

VulkanRecordingUploadPage::~VulkanRecordingUploadPage() noexcept
{
	if (m_buffer != nullptr)
	{
		m_memoryAllocator->QueueDestroyResource(std::move(m_buffer));
	}
}

void VulkanRecordingUploadPage::Reset() noexcept
{
	m_allocationCount = 0;
	m_offset = 0;
}

RhiGpuVirtualAddress VulkanRecordingUploadPage::AllocateAndCopy(const void* data, std::uint32_t sizeInBytes) noexcept
{
	if (m_buffer == nullptr || m_buffer->Buffer == VK_NULL_HANDLE || data == nullptr || sizeInBytes == 0 || m_mappedData == nullptr
	    || m_allocationCount >= MaximumAllocations)
	{
		return {};
	}

	const VkDeviceSize alignedOffset = (m_offset + m_alignment - 1) & ~(m_alignment - 1);
	if (alignedOffset > m_capacityInBytes || sizeInBytes > m_capacityInBytes - alignedOffset)
	{
		return {};
	}
	std::memcpy(m_mappedData + alignedOffset, data, sizeInBytes);
	if (!m_memoryAllocator->FlushUploadPage(*m_buffer, alignedOffset, sizeInBytes))
	{
		return {};
	}

	const std::size_t allocationIndex = m_allocationCount++;
	m_allocations[allocationIndex] = Allocation{.Offset = alignedOffset, .Size = sizeInBytes};
	m_offset = alignedOffset + sizeInBytes;
	return reinterpret_cast<RhiGpuVirtualAddress>(&m_allocations[allocationIndex]);
}

bool VulkanRecordingUploadPage::Resolve(RhiGpuVirtualAddress address, VkBuffer& buffer, VkDeviceSize& offset, VkDeviceSize& range)
    const noexcept
{
	if (address == 0 || m_buffer == nullptr)
	{
		return false;
	}

	const std::uintptr_t allocationAddress = static_cast<std::uintptr_t>(address);
	const std::uintptr_t allocationBegin = reinterpret_cast<std::uintptr_t>(m_allocations.data());
	const std::uintptr_t allocationEnd = allocationBegin + m_allocationCount * sizeof(Allocation);
	if (allocationAddress < allocationBegin || allocationAddress >= allocationEnd
	    || (allocationAddress - allocationBegin) % sizeof(Allocation) != 0)
	{
		return false;
	}

	const std::size_t allocationIndex = (allocationAddress - allocationBegin) / sizeof(Allocation);
	const Allocation& allocation = m_allocations[allocationIndex];
	buffer = m_buffer->Buffer;
	offset = allocation.Offset;
	range = allocation.Size;
	return true;
}
