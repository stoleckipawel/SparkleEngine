#pragma once

#include "Descriptors/RhiDescriptorService.h"
#include "Frame/RhiFrameConstants.h"
#include "Resources/RhiResourceView.h"
#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <vector>

class VulkanRhi;
class VulkanSwapChain;
class VulkanGpuMemoryAllocator;
class VulkanSamplerLibrary;
struct RhiCapabilities;

class VulkanDescriptorManager final : public RhiDescriptorService
{
  public:
	VulkanDescriptorManager(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator, const RhiCapabilities& capabilities) noexcept;
	~VulkanDescriptorManager() noexcept;

	VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;
	VulkanDescriptorManager(VulkanDescriptorManager&&) = delete;
	VulkanDescriptorManager& operator=(VulkanDescriptorManager&&) = delete;

	VulkanDescriptorAllocator& GetAllocator() noexcept { return m_allocator; }
	const VulkanDescriptorAllocator& GetAllocator() const noexcept { return m_allocator; }
	void SetSamplerLibrary(VulkanSamplerLibrary& samplerLibrary) noexcept;

	void BeginFrame(std::uint32_t frameIndex) noexcept override;
	std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc) override;
	void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept override;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType) override;
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept override;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount) override;
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept override;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept override;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) override;
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept override;
	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept override;

	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) override;
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept override;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept override;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept override;
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept override;
	VkImageView GetRegisteredImageView(RhiGpuDescriptorHandle descriptorHandle) const noexcept;
	VkImageAspectFlags ResolveImageViewAspectMask(VkImageView imageView) const noexcept;
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

		bool IsAllocated() const noexcept
		{
			return Image != VK_NULL_HANDLE || Buffer != VK_NULL_HANDLE || AccelerationStructure != VK_NULL_HANDLE ||
			       ImageView != VK_NULL_HANDLE || static_cast<bool>(DescriptorHandle);
		}
	};

	static RhiResourceViewHandle MakeResourceViewHandle(std::uint32_t index) noexcept;
	RhiResourceViewHandle AddResourceView(ResourceViewRecord record);
	ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) noexcept;
	const ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) const noexcept;
	VkImageView CreateImageView(const RhiResourceViewDesc& desc) const;
	VkFormat ResolveViewFormat(const RhiResourceViewDesc& desc) const noexcept;
	VkImageAspectFlags ResolveViewAspectMask(const RhiResourceViewDesc& desc) const noexcept;
	void DestroyResourceView(ResourceViewRecord& record) noexcept;

	VulkanRhi& m_rhi;
	VulkanGpuMemoryAllocator& m_memoryAllocator;
	const RhiCapabilities& m_capabilities;
	VulkanDescriptorAllocator m_allocator;
	VulkanSamplerLibrary* m_samplerLibrary = nullptr;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
	std::array<std::vector<ResourceViewRecord>, RhiFrameConstants::FramesInFlight> m_retiredResourceViews;
	std::vector<RhiResourceViewHandle> m_swapChainBackBufferViews;
	std::uint32_t m_currentFrameIndex = 0;
};
