#pragma once

#include "Resources/RhiUploadService.h"
#include "Vulkan/Resources/VulkanLinearAllocator.h"

class VulkanCommandContext;
class VulkanGpuMemoryAllocator;

class VulkanUploadService final : public RhiUploadService
{
  public:
	VulkanUploadService(VulkanCommandContext& commandContext, VulkanGpuMemoryAllocator& memoryAllocator);
	~VulkanUploadService() noexcept = default;

	VulkanUploadService(const VulkanUploadService&) = delete;
	VulkanUploadService& operator=(const VulkanUploadService&) = delete;
	VulkanUploadService(VulkanUploadService&&) = delete;
	VulkanUploadService& operator=(VulkanUploadService&&) = delete;

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override;
	bool UploadTexture(
	    RenderCommandList& commandList,
	    RhiOwnedResourceHandle destination,
	    const RhiTextureUploadDesc& textureUpload,
	    ResourceState finalState,
	    std::wstring_view debugName) override;

  private:
	VulkanCommandContext* m_commandContext = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanLinearAllocator m_uniformAllocator;
};
