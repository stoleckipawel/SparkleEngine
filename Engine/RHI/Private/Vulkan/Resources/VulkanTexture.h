#pragma once

#include "Resources/Texture.h"
#include "Resources/RhiTextureUpload.h"
#include "Vulkan/VulkanIncludes.h"

#include <memory>

class VulkanDescriptorManager;
class VulkanGpuMemoryAllocator;
struct VulkanGpuAllocationRecord;
class VulkanRhi;

class VulkanTexture final : public Texture
{
  public:
	VulkanTexture(
	    VulkanRhi& rhi,
	    VulkanGpuMemoryAllocator& memoryAllocator,
	    VulkanDescriptorManager& descriptorManager,
	    RhiTextureUploadDesc textureUpload,
	    std::wstring_view debugName);
	~VulkanTexture() noexcept override;

	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture& operator=(const VulkanTexture&) = delete;
	VulkanTexture(VulkanTexture&&) = delete;
	VulkanTexture& operator=(VulkanTexture&&) = delete;

	NativeResourceHandle GetNativeResource() const noexcept override;
	void WriteShaderResourceView(RhiCpuDescriptorHandle destination) const override;
	TextureRuntimeInfo GetRuntimeInfo() const noexcept override;

  private:
	void CreateImage(std::wstring_view debugName);
	void UploadImage(std::wstring_view debugName);
	void CreateShaderResourceView();
	VkImageViewCreateInfo BuildImageViewCreateInfo() const noexcept;

	VulkanRhi& m_rhi;
	VulkanGpuMemoryAllocator& m_memoryAllocator;
	VulkanDescriptorManager& m_descriptorManager;
	RhiTextureUploadDesc m_textureUpload;
	std::unique_ptr<VulkanGpuAllocationRecord> m_imageAllocation;
	std::unique_ptr<VulkanGpuAllocationRecord> m_uploadAllocation;
	VkImageView m_imageView = VK_NULL_HANDLE;
	RhiGpuDescriptorHandle m_shaderResourceDescriptor = {};
};
