#pragma once

#include "Core/RhiCapabilities.h"
#include "Resources/RhiResourceService.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class D3D12DescriptorService;
class D3D12GpuMemoryAllocator;
class D3D12RenderHardwareInterface;
class D3D12Rhi;
struct D3D12GpuAllocationRecord;
struct D3D12GpuHeapRecord;
struct ID3D12Resource;

class D3D12ResourceService final : public RhiResourceService
{
  public:
	D3D12ResourceService(
	    D3D12Rhi& rhi,
	    D3D12GpuMemoryAllocator& memoryAllocator,
	    D3D12DescriptorService& descriptorService,
	    const RhiCapabilities& capabilities) noexcept;

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
	bool CreateStructuredBufferResource(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource) override;
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
	friend class D3D12RenderHardwareInterface;
	void DrainCompletedResourceReleases() noexcept;
	void FlushDeferredResourceReleases() noexcept;
	void BeginResourceTracking(RhiResourceHandle resource) noexcept;
	void EndResourceTracking(RhiResourceHandle resource, RhiSubmissionToken submissionToken) noexcept;

	struct PendingOwnedResourceRelease
	{
		std::unique_ptr<D3D12GpuAllocationRecord> Record;
	};

	struct PendingOwnedMemoryBlockRelease
	{
		std::unique_ptr<D3D12GpuHeapRecord> Record;
	};

	static std::wstring CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName);
	static RhiOwnedResourceHandle WrapOwnedResource(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept;
	static RhiOwnedMemoryBlockHandle WrapOwnedMemoryBlock(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept;
	static bool ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept;
	void CollectCrashDiagnosticsOnce() noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	D3D12DescriptorService* m_descriptorService = nullptr;
	const RhiCapabilities* m_capabilities = nullptr;
	std::vector<PendingOwnedResourceRelease> m_pendingOwnedResourceReleases;
	std::vector<PendingOwnedMemoryBlockRelease> m_pendingOwnedMemoryBlockReleases;
	bool m_crashDiagnosticsCollected = false;
};
