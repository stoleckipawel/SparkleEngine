#pragma once

#include "Resources/RenderConstantBufferData.h"
#include "Resources/RhiResourceDesc.h"
#include "Vulkan/Resources/VulkanLinearAllocator.h"

class VulkanGpuMemoryAllocator;

class VulkanConstantBufferManager final
{
  public:
	explicit VulkanConstantBufferManager(VulkanGpuMemoryAllocator& memoryAllocator);
	~VulkanConstantBufferManager() noexcept = default;

	VulkanConstantBufferManager(const VulkanConstantBufferManager&) = delete;
	VulkanConstantBufferManager& operator=(const VulkanConstantBufferManager&) = delete;
	VulkanConstantBufferManager(VulkanConstantBufferManager&&) = delete;
	VulkanConstantBufferManager& operator=(VulkanConstantBufferManager&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	const PerFrameConstantBufferData& GetPerFrameData() const noexcept;
	RhiGpuVirtualAddress GetPerFrameGpuAddress() const noexcept;
	RhiGpuVirtualAddress AllocateUniform(const void* data, std::uint32_t sizeInBytes);
	RhiGpuVirtualAddress AllocatePerView(const PerViewConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data);

  private:
	PerFrameConstantBufferData m_emptyPerFrameConstants = {};
	VulkanLinearAllocator m_uniformAllocator;
};