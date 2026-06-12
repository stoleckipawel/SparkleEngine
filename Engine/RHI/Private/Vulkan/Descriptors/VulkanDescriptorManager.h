#pragma once

#include "Resources/RhiResourceView.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <vector>

class VulkanRhi;
class VulkanSwapChain;
class VulkanGpuMemoryAllocator;

class VulkanDescriptorManager final
{
  public:
	VulkanDescriptorManager(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept;
	~VulkanDescriptorManager() noexcept;

	VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager(VulkanDescriptorManager&&) = delete;
	VulkanDescriptorManager& operator=(VulkanDescriptorManager&&) = delete;

	VulkanDescriptorAllocator& GetAllocator() noexcept { return m_allocator; }
	const VulkanDescriptorAllocator& GetAllocator() const noexcept { return m_allocator; }

	void BeginFrame(std::uint32_t frameIndex) noexcept;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType);
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount);
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept;

	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc);
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept;
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept;
	VkImageView GetRegisteredImageView(RhiGpuDescriptorHandle descriptorHandle) const noexcept;
	void RebuildSwapChainBackBufferViews(const VulkanSwapChain& swapChain) noexcept;
	RhiResourceViewHandle GetSwapChainBackBufferView(std::uint32_t backBufferIndex) const noexcept;
	void ReleaseAllResourceViews() noexcept;

  private:
	struct ResourceViewRecord final
	{
		ERhiResourceViewKind Kind = ERhiResourceViewKind::TextureShaderResource;
		VkImage Image = VK_NULL_HANDLE;
		VkBuffer Buffer = VK_NULL_HANDLE;
		VkAccelerationStructureKHR AccelerationStructure = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		PixelFormat Format = PixelFormat::Unknown;
		RhiTextureViewRange Texture = {};
		VkImageUsageFlags Usage = 0;
		RhiGpuDescriptorHandle DescriptorHandle = {};
		bool OwnsImageView = false;
	};

	static RhiResourceViewHandle MakeResourceViewHandle(std::uint32_t index) noexcept;
	RhiResourceViewHandle AddResourceView(ResourceViewRecord record);
	ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) noexcept;
	const ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) const noexcept;
	VkImageView CreateImageView(const RhiResourceViewDesc& desc) const;
	VkFormat ResolveViewFormat(const RhiResourceViewDesc& desc) const noexcept;
	VkImageAspectFlags ResolveViewAspectMask(const RhiResourceViewDesc& desc) const noexcept;

	VulkanRhi& m_rhi;
	VulkanGpuMemoryAllocator& m_memoryAllocator;
	VulkanDescriptorAllocator m_allocator;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
	std::vector<RhiResourceViewHandle> m_swapChainBackBufferViews;
};
