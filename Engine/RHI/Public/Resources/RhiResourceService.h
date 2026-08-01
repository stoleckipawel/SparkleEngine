#pragma once

#include "../Memory/RhiMemoryTypes.h"
#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceView.h"
#include "../RHIAPI.h"
#include "RhiResourceHandles.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class SPARKLE_RHI_API RhiResourceService
{
  public:
	virtual ~RhiResourceService() noexcept = default;

	virtual RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateBufferResource(
	    const RhiBufferResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) = 0;
	virtual bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView) = 0;
	virtual bool CreateStructuredBufferResource(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource) = 0;
	virtual bool WriteBufferResource(
	    RhiOwnedResourceHandle resource,
	    std::size_t destinationOffsetInBytes,
	    const void* data,
	    std::size_t sizeInBytes) noexcept = 0;
	virtual bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView) = 0;
	virtual void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept = 0;
	virtual RhiResourceHandle GetResourceHandle(RhiOwnedResourceHandle resource) const noexcept = 0;
	virtual RhiGpuVirtualAddress GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept = 0;
	virtual RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept = 0;
	virtual RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept = 0;
	virtual RhiOwnedMemoryBlockHandle CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) = 0;
	virtual void ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept = 0;
	virtual RhiOwnedResourceHandle CreateAliasingTextureResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName) = 0;
	virtual RhiOwnedResourceHandle CreateAliasingBufferResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName) = 0;
	virtual bool SupportsUnorderedAccess(RhiResourceHandle resource) const noexcept = 0;
};
