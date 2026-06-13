#pragma once

#include "Config/RenderConfig.h"
#include "Resources/RenderConstantBufferData.h"
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
	const PerFrameConstantBufferData& GetPerFrameData() const noexcept;
	const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept override { return GetPerFrameData(); }
	RhiGpuVirtualAddress GetPerFrameGpuAddress() const noexcept;
	RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept override { return GetPerFrameGpuAddress(); }
	void UpdatePerFrame(const PerFrameConstantBufferData& data) noexcept;
	RhiGpuVirtualAddress AllocateUniform(const void* data, std::uint32_t sizeInBytes);
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override
	{
		return AllocateUniform(data, sizeInBytes);
	}
	RhiGpuVirtualAddress AllocatePerView(const PerViewConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) override { return AllocatePerView(data); }
	RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) override;
	RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) override;

  private:
	PerFrameConstantBufferData m_perFrameData[RenderConfig::FramesInFlight] = {};
	VulkanLinearAllocator m_uniformAllocator;
	std::uint32_t m_currentFrameIndex = 0;
};
