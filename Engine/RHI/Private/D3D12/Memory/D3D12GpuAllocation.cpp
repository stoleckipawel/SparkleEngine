#include "PCH.h"

#include "D3D12/Memory/D3D12GpuAllocation.h"

#include <D3D12MemAlloc.h>

D3D12GpuAllocationRecord::~D3D12GpuAllocationRecord() noexcept
{
	if (IsMapped && Resource != nullptr)
	{
		Resource->Unmap(0, nullptr);
		IsMapped = false;
		CpuMappedAddress = nullptr;
	}

	Resource.Reset();
	if (Allocation != nullptr)
	{
		Allocation->Release();
		Allocation = nullptr;
	}
}

D3D12GpuHeapRecord::~D3D12GpuHeapRecord() noexcept
{
	NativeHeap.Reset();
	if (Allocation != nullptr)
	{
		Allocation->Release();
		Allocation = nullptr;
	}
	if (Pool != nullptr)
	{
		Pool->Release();
		Pool = nullptr;
	}
}

RhiOwnedResourceHandle MakeD3D12OwnedResourceHandle(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept
{
	return RhiOwnedResourceHandle{record.release()};
}

std::unique_ptr<D3D12GpuAllocationRecord> TakeD3D12OwnedResourceHandle(RhiOwnedResourceHandle handle) noexcept
{
	return std::unique_ptr<D3D12GpuAllocationRecord>(static_cast<D3D12GpuAllocationRecord*>(handle.Value));
}

D3D12GpuAllocationRecord* GetD3D12GpuAllocationRecord(RhiOwnedResourceHandle handle) noexcept
{
	return static_cast<D3D12GpuAllocationRecord*>(handle.Value);
}

ID3D12Resource* GetD3D12Resource(RhiOwnedResourceHandle handle) noexcept
{
	D3D12GpuAllocationRecord* const record = GetD3D12GpuAllocationRecord(handle);
	return record != nullptr ? record->Resource.Get() : nullptr;
}

RhiOwnedHeapHandle MakeD3D12OwnedHeapHandle(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept
{
	return RhiOwnedHeapHandle{record.release()};
}

std::unique_ptr<D3D12GpuHeapRecord> TakeD3D12OwnedHeapHandle(RhiOwnedHeapHandle handle) noexcept
{
	return std::unique_ptr<D3D12GpuHeapRecord>(static_cast<D3D12GpuHeapRecord*>(handle.Value));
}

D3D12GpuHeapRecord* GetD3D12GpuHeapRecord(RhiOwnedHeapHandle handle) noexcept
{
	return static_cast<D3D12GpuHeapRecord*>(handle.Value);
}

ID3D12Heap* GetD3D12Heap(RhiOwnedHeapHandle handle) noexcept
{
	D3D12GpuHeapRecord* const record = GetD3D12GpuHeapRecord(handle);
	return record != nullptr ? record->NativeHeap.Get() : nullptr;
}
