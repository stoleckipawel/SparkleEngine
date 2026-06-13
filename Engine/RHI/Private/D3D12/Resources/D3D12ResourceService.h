#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Memory/RhiMemoryTypes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12DescriptorService;
class D3D12GpuMemoryAllocator;
class D3D12Rhi;
class Texture;
struct D3D12GpuAllocationRecord;
struct D3D12GpuHeapRecord;
struct ID3D12Resource;

class D3D12ResourceService final
{
  public:
	D3D12ResourceService(
	    D3D12Rhi& rhi,
	    D3D12GpuMemoryAllocator& memoryAllocator,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12DescriptorService& descriptorService,
	    const RhiCapabilities& capabilities) noexcept;

	std::unique_ptr<Texture> CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateBufferResource(
	    const RhiBufferResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName);
	bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView);
	bool CreateStructuredBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiResourceViewHandle& outView);
	bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView);
	void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept;
	void DrainCompletedReleases() noexcept;
	NativeResourceHandle GetNativeResource(RhiOwnedResourceHandle resource) const noexcept;
	RhiGpuVirtualAddress GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept;
	RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept;
	RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept;
	RhiOwnedMemoryBlockHandle CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName);
	void ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept;
	RhiOwnedResourceHandle CreateAliasingTextureResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateAliasingBufferResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName);
	bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept;

  private:
	struct PendingOwnedResourceRelease
	{
		std::unique_ptr<D3D12GpuAllocationRecord> Record;
		std::uint64_t RetireFenceValue = 0;
	};

	struct PendingOwnedMemoryBlockRelease
	{
		std::unique_ptr<D3D12GpuHeapRecord> Record;
		std::uint64_t RetireFenceValue = 0;
	};

	static std::wstring CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName);
	static RhiOwnedResourceHandle WrapOwnedResource(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept;
	static RhiOwnedMemoryBlockHandle WrapOwnedMemoryBlock(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept;
	static bool ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12DescriptorService* m_descriptorService = nullptr;
	const RhiCapabilities* m_capabilities = nullptr;
	std::vector<PendingOwnedResourceRelease> m_pendingOwnedResourceReleases;
	std::vector<PendingOwnedMemoryBlockRelease> m_pendingOwnedMemoryBlockReleases;
};
