#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanConstantBufferManager.h"

VulkanConstantBufferManager::VulkanConstantBufferManager(VulkanGpuMemoryAllocator& memoryAllocator) : m_uniformAllocator(memoryAllocator) {}

void VulkanConstantBufferManager::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_uniformAllocator.BeginFrame(frameIndex);
}

const PerFrameConstantBufferData& VulkanConstantBufferManager::GetPerFrameData() const noexcept
{
	return m_emptyPerFrameConstants;
}

RhiGpuVirtualAddress VulkanConstantBufferManager::GetPerFrameGpuAddress() const noexcept
{
	return {};
}

RhiGpuVirtualAddress VulkanConstantBufferManager::AllocateUniform(const void* data, std::uint32_t sizeInBytes)
{
	return m_uniformAllocator.AllocateAndCopy(data, sizeInBytes);
}

RhiGpuVirtualAddress VulkanConstantBufferManager::AllocatePerView(const PerViewConstantBufferData& data)
{
	return AllocateUniform(&data, sizeof(data));
}

RhiGpuVirtualAddress VulkanConstantBufferManager::AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data)
{
	return AllocateUniform(&data, sizeof(data));
}

RhiGpuVirtualAddress VulkanConstantBufferManager::AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data)
{
	return AllocateUniform(&data, sizeof(data));
}