#pragma once

#include "Core/RhiCapabilities.h"
#include "Resources/RhiResourceService.h"

#include <cstdint>
#include <memory>
#include <string_view>

class VulkanGpuMemoryAllocator;
class VulkanRenderHardwareInterface;
class VulkanRhi;

class VulkanResourceService final : public RhiResourceService
{
  public:
	VulkanResourceService(
	    VulkanRhi& rhi,
	    VulkanGpuMemoryAllocator& memoryAllocator,
	    const RhiCapabilities& capabilities) noexcept;
	~VulkanResourceService() noexcept;

	VulkanResourceService(const VulkanResourceService&) = delete;
	VulkanResourceService& operator=(const VulkanResourceService&) = delete;
	VulkanResourceService(VulkanResourceService&&) = delete;
	VulkanResourceService& operator=(VulkanResourceService&&) = delete;

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
	bool CreateStructuredBufferResource(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource) override;
	bool WriteBufferResource(
	    RhiOwnedResourceHandle resource,
	    std::size_t destinationOffsetInBytes,
	    const void* data,
	    std::size_t sizeInBytes) noexcept override;
	bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView) override;
	void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept override;
	RhiResourceHandle GetResourceHandle(RhiOwnedResourceHandle resource) const noexcept override;
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
	bool SupportsUnorderedAccess(RhiResourceHandle resource) const noexcept override;

  private:
	friend class VulkanRenderHardwareInterface;
	void DrainCompletedResourceReleases() noexcept;
	void FlushDeferredResourceReleases() noexcept;

	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	const RhiCapabilities* m_capabilities = nullptr;
};
