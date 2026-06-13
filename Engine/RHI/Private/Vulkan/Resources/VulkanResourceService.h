#pragma once

#include "Core/RhiCapabilities.h"
#include "Resources/RhiResourceService.h"

#include <cstdint>
#include <memory>
#include <string_view>

class Texture;
class VulkanCommandContext;
class VulkanDescriptorManager;
class VulkanGpuMemoryAllocator;
class VulkanRhi;
class VulkanTextureFactory;

class VulkanResourceService final : public RhiResourceService
{
  public:
	VulkanResourceService(
	    VulkanRhi& rhi,
	    VulkanCommandContext& commandContext,
	    VulkanGpuMemoryAllocator& memoryAllocator,
	    VulkanDescriptorManager& descriptorManager,
	    const RhiCapabilities& capabilities) noexcept;
	~VulkanResourceService() noexcept;

	VulkanResourceService(const VulkanResourceService&) = delete;
	VulkanResourceService& operator=(const VulkanResourceService&) = delete;
	VulkanResourceService(VulkanResourceService&&) = delete;
	VulkanResourceService& operator=(VulkanResourceService&&) = delete;

	std::unique_ptr<Texture> CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateBufferResource(
	    const RhiBufferResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) override;
	bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView) override;
	bool CreateStructuredBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiResourceViewHandle& outView) override;
	bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView) override;
	void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept override;
	void FlushPendingReleases() noexcept;
	void DrainCompletedReleases() noexcept;
	NativeResourceHandle GetNativeResource(RhiOwnedResourceHandle resource) const noexcept override;
	RhiGpuVirtualAddress GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept override;
	RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept override;
	RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept override;
	RhiOwnedMemoryBlockHandle CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) override;
	void ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept override;
	RhiOwnedResourceHandle CreateAliasingTextureResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateAliasingBufferResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName) override;
	bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept override;

  private:
	VulkanRhi* m_rhi = nullptr;
	VulkanCommandContext* m_commandContext = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanDescriptorManager* m_descriptorManager = nullptr;
	const RhiCapabilities* m_capabilities = nullptr;
	std::unique_ptr<VulkanTextureFactory> m_textureFactory;
};
