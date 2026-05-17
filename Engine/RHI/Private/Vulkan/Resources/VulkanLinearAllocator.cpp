#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanLinearAllocator.h"

#include "Config/RenderConfig.h"
#include "Resources/RhiResourceDesc.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

VulkanLinearAllocator::VulkanLinearAllocator(VulkanGpuMemoryAllocator& memoryAllocator) : m_memoryAllocator(memoryAllocator)
{
	m_uploadRecordsByFrame.resize(RenderConfig::FramesInFlight);
}

void VulkanLinearAllocator::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_currentFrameIndex = frameIndex;
	if (m_uploadRecordsByFrame.size() <= frameIndex)
	{
		m_uploadRecordsByFrame.resize(static_cast<std::size_t>(frameIndex) + 1u);
	}
	m_uploadRecordsByFrame[frameIndex].clear();
}

RhiGpuVirtualAddress VulkanLinearAllocator::AllocateAndCopy(const void* data, std::uint32_t sizeInBytes)
{
	if (data == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .StrideInBytes = 0, .AllowUnorderedAccess = false};
	const VkBufferCreateInfo createInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator.CreateBuffer(
	    createInfo,
	    RhiMemoryCategory::ConstantBuffer,
	    RhiMemoryResidencyClass::HostUpload,
	    L"Vulkan Uniform Upload");
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator.WriteAllocation(*record, data, sizeInBytes))
	{
		return {};
	}

	const VkBuffer buffer = record->Buffer;
	if (m_currentFrameIndex < m_uploadRecordsByFrame.size())
	{
		m_uploadRecordsByFrame[m_currentFrameIndex].push_back(std::move(record));
	}
	return reinterpret_cast<RhiGpuVirtualAddress>(buffer);
}