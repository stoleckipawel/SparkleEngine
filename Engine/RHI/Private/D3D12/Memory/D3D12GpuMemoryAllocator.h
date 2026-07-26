#pragma once

#include "Core/Public/Threading/ThreadOwnership.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12RecordingResourceUseToken.h"
#include "Memory/RhiMemoryDiagnostics.h"
#include "Memory/RhiMemoryTypes.h"
#include "Resources/RhiResourceDesc.h"

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
	bool SupportsBudgetQueries() const noexcept;
	RhiMemoryUsageSnapshot CreateMemoryUsageSnapshot() const;
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
	std::unique_ptr<D3D12GpuHeapRecord> CreateTransientHeap(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateAliasingTexture(
	    D3D12GpuHeapRecord& heap,
	    std::uint64_t heapOffset,
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    const D3D12_CLEAR_VALUE* optimizedClearValue,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateAliasingBuffer(
	    D3D12GpuHeapRecord& heap,
	    std::uint64_t heapOffset,
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    std::wstring_view debugName) noexcept;
  private:
	friend class D3D12RenderDeviceServices;
	friend class D3D12ResourceService;

	struct Impl;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateResource(
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    const D3D12_CLEAR_VALUE* optimizedClearValue,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) noexcept;
	std::unique_ptr<D3D12GpuAllocationRecord> CreateAliasingResource(
	    D3D12GpuHeapRecord& heap,
	    std::uint64_t heapOffset,
	    const D3D12_RESOURCE_DESC& resourceDesc,
	    D3D12_RESOURCE_STATES initialState,
	    const D3D12_CLEAR_VALUE* optimizedClearValue,
	    std::wstring_view debugName) noexcept;
	static D3D12_HEAP_TYPE ToHeapType(RhiMemoryResidencyClass residencyClass) noexcept;
	static D3D12_HEAP_FLAGS ToTransientHeapFlags(RhiTransientAllocationPool pool) noexcept;
	void RegisterAllocationRecord(D3D12GpuAllocationRecord& record) noexcept;
	void UnregisterAllocationRecord(D3D12GpuAllocationRecord& record) noexcept;
	void RegisterHeapRecord(D3D12GpuHeapRecord& record) noexcept;
	void UnregisterHeapRecord(D3D12GpuHeapRecord& record) noexcept;
	D3D12GpuAllocationRecord* FindAllocationRecord(
	    ID3D12Resource* resource) const noexcept;
	void PublishRecordingReadView() noexcept;
	D3D12RecordingResourceUseToken RetainRecordingResource(
	    RhiResourceHandle resource) const noexcept;
	D3D12RecordingResourceUseToken RetainCoordinatorRecordingResource(
	    RhiResourceHandle resource) const noexcept;
	void ReleaseRecordingResource(
	    D3D12RecordingResourceUseToken use,
	    RhiSubmissionToken submissionToken) const noexcept;

	friend struct D3D12GpuAllocationRecord;
	friend struct D3D12GpuHeapRecord;

	Threading::OwnerThread m_owner{"D3D12 GPU memory allocator"};
	std::unique_ptr<Impl> m_impl;
	std::unique_ptr<class D3D12RecordingResourceTable> m_recordingResources;
};
