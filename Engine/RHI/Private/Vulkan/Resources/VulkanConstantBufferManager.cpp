#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Resources/VulkanConstantBufferManager.h"

VulkanConstantBufferManager::VulkanConstantBufferManager(VulkanGpuMemoryAllocator& memoryAllocator) : m_uniformAllocator(memoryAllocator) {}

void VulkanConstantBufferManager::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_currentFrameIndex = frameIndex % RenderConfig::FramesInFlight;
	m_uniformAllocator.BeginFrame(frameIndex);
}

const PerFrameConstantBufferData& VulkanConstantBufferManager::GetPerFrameData() const noexcept
{
	return m_perFrameData[m_currentFrameIndex];
}

RhiGpuVirtualAddress VulkanConstantBufferManager::GetPerFrameGpuAddress() const noexcept
{
	return {};
}

void VulkanConstantBufferManager::UpdatePerFrame(const PerFrameConstantBufferData& data) noexcept
{
	m_perFrameData[m_currentFrameIndex] = data;
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
