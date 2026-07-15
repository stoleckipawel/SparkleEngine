#pragma once

#include "Interop/RhiNativeHandles.h"
#include "Memory/RhiMemoryTypes.h"
#include "Commands/RhiQueue.h"

#include <d3d12.h>
#include <wrl/client.h>

#include <memory>
#include <string>
#include <string_view>

namespace D3D12MA
{
	class Allocation;
	class Pool;
}

class D3D12GpuMemoryAllocator;
struct D3D12GpuHeapRecord;

struct D3D12GpuAllocationRecord final
{
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	D3D12MA::Allocation* Allocation = nullptr;
	D3D12MA::Pool* Pool = nullptr;
	D3D12GpuHeapRecord* ParentHeap = nullptr;
	RhiMemoryCategory Category = RhiMemoryCategory::Other;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::DeviceLocal;
	std::wstring DebugName;
	D3D12GpuMemoryAllocator* Owner = nullptr;
	bool IsMapped = false;
	void* CpuMappedAddress = nullptr;
	RhiSubmissionState LastUse;
	std::uint32_t RecordingReferenceCount = 0;

	D3D12GpuAllocationRecord() noexcept = default;
	~D3D12GpuAllocationRecord() noexcept;

	D3D12GpuAllocationRecord(const D3D12GpuAllocationRecord&) = delete;
	D3D12GpuAllocationRecord& operator=(const D3D12GpuAllocationRecord&) = delete;
	D3D12GpuAllocationRecord(D3D12GpuAllocationRecord&&) = delete;
	D3D12GpuAllocationRecord& operator=(D3D12GpuAllocationRecord&&) = delete;
};

struct D3D12GpuHeapRecord final
{
	Microsoft::WRL::ComPtr<ID3D12Heap> NativeHeap;
	D3D12MA::Allocation* Allocation = nullptr;
	D3D12MA::Pool* Pool = nullptr;
	RhiMemoryCategory Category = RhiMemoryCategory::TransientResource;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::Transient;
	std::wstring DebugName;
	D3D12GpuMemoryAllocator* Owner = nullptr;
	std::uint32_t AliasingResourceCount = 0;
	RhiSubmissionState LastUse;
	std::uint32_t RecordingReferenceCount = 0;

	D3D12GpuHeapRecord() noexcept = default;
	~D3D12GpuHeapRecord() noexcept;

	D3D12GpuHeapRecord(const D3D12GpuHeapRecord&) = delete;
	D3D12GpuHeapRecord& operator=(const D3D12GpuHeapRecord&) = delete;
	D3D12GpuHeapRecord(D3D12GpuHeapRecord&&) = delete;
	D3D12GpuHeapRecord& operator=(D3D12GpuHeapRecord&&) = delete;
};

RhiOwnedResourceHandle MakeD3D12OwnedResourceHandle(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept;
std::unique_ptr<D3D12GpuAllocationRecord> TakeD3D12OwnedResourceHandle(RhiOwnedResourceHandle handle) noexcept;
D3D12GpuAllocationRecord* GetD3D12GpuAllocationRecord(RhiOwnedResourceHandle handle) noexcept;
ID3D12Resource* GetD3D12Resource(RhiOwnedResourceHandle handle) noexcept;
void SetD3D12AllocationRecordDebugName(D3D12GpuAllocationRecord& record, std::wstring_view debugName) noexcept;

RhiOwnedMemoryBlockHandle MakeD3D12OwnedMemoryBlockHandle(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept;
std::unique_ptr<D3D12GpuHeapRecord> TakeD3D12OwnedMemoryBlockHandle(RhiOwnedMemoryBlockHandle handle) noexcept;
D3D12GpuHeapRecord* GetD3D12GpuHeapRecord(RhiOwnedMemoryBlockHandle handle) noexcept;
ID3D12Heap* GetD3D12Heap(RhiOwnedMemoryBlockHandle handle) noexcept;
