#pragma once

#include "Resources/RhiResourceDesc.h"
#include "Resources/RhiUploadService.h"
#include "Vulkan/Resources/VulkanLinearAllocator.h"

class VulkanGpuMemoryAllocator;

class VulkanConstantBufferManager final : public RhiUploadService
{
  public:
	explicit VulkanConstantBufferManager(VulkanGpuMemoryAllocator& memoryAllocator);
	~VulkanConstantBufferManager() noexcept = default;

	VulkanConstantBufferManager(const VulkanConstantBufferManager&) = delete;
	VulkanConstantBufferManager& operator=(const VulkanConstantBufferManager&) = delete;
	VulkanConstantBufferManager(VulkanConstantBufferManager&&) = delete;
	VulkanConstantBufferManager& operator=(VulkanConstantBufferManager&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RhiGpuVirtualAddress AllocateUniform(const void* data, std::uint32_t sizeInBytes);
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override
	{
		return AllocateUniform(data, sizeInBytes);
	}

  private:
	VulkanLinearAllocator m_uniformAllocator;
};
