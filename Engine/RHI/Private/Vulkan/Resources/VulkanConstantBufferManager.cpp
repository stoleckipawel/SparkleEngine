#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanConstantBufferManager.h"

VulkanConstantBufferManager::VulkanConstantBufferManager(VulkanGpuMemoryAllocator& memoryAllocator) : m_uniformAllocator(memoryAllocator) {}

void VulkanConstantBufferManager::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_uniformAllocator.BeginFrame(frameIndex);
}

RhiGpuVirtualAddress VulkanConstantBufferManager::AllocateUniform(const void* data, std::uint32_t sizeInBytes)
{
	return m_uniformAllocator.AllocateAndCopy(data, sizeInBytes);
}

