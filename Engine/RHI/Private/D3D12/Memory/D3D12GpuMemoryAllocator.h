#pragma once

#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "Memory/RhiMemoryTypes.h"

#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string_view>

struct ID3D12Device;
struct IDXGIAdapter;

class D3D12GpuMemoryAllocator final
{
  public:
	explicit D3D12GpuMemoryAllocator(IDXGIAdapter* adapter, ID3D12Device* device) noexcept;
	~D3D12GpuMemoryAllocator() noexcept;

	D3D12GpuMemoryAllocator(const D3D12GpuMemoryAllocator&) = delete;
	D3D12GpuMemoryAllocator& operator=(const D3D12GpuMemoryAllocator&) = delete;
	D3D12GpuMemoryAllocator(D3D12GpuMemoryAllocator&&) = delete;
	D3D12GpuMemoryAllocator& operator=(D3D12GpuMemoryAllocator&&) = delete;

	bool IsInitialized() const noexcept;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateTexture(
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    const D3D12_CLEAR_VALUE* optimizedClearValue,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateBuffer(
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;

  private:
	struct Impl;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateResource(
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    const D3D12_CLEAR_VALUE* optimizedClearValue,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	static D3D12_HEAP_TYPE ToHeapType(RhiMemoryResidencyClass residencyClass) noexcept;

	std::unique_ptr<Impl> m_impl;
};
