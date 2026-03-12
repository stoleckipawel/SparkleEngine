#include "PCH.h"

#include "D3D12LinearAllocator.h"
#include "D3D12Rhi.h"

#include <cstring>

void D3D12LinearAllocator::Initialize(D3D12Rhi& rhi, uint64_t capacity, const wchar_t* debugName)
{
	assert(capacity > 0);

	m_rhi = &rhi;
	m_Capacity = capacity;
	m_Offset.store(0, std::memory_order_relaxed);

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = capacity;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = m_rhi->GetDevice()->CreateCommittedResource(
	    &heapProps,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(&m_Resource));

	if (FAILED(hr))
	{
		throw std::runtime_error("D3D12LinearAllocator: Failed to create upload buffer");
	}

	DebugUtils::SetDebugName(m_Resource, debugName);

	D3D12_RANGE readRange = {0, 0};
	hr = m_Resource->Map(0, &readRange, reinterpret_cast<void**>(&m_CpuBase));
	if (FAILED(hr))
	{
		throw std::runtime_error("D3D12LinearAllocator: Failed to map upload buffer");
	}

	m_GpuBase = m_Resource->GetGPUVirtualAddress();
	m_bInitialized = true;
}

void D3D12LinearAllocator::Shutdown()
{
	if (m_Resource)
	{
		m_Resource->Unmap(0, nullptr);
		m_Resource.Reset();
	}
	m_CpuBase = nullptr;
	m_GpuBase = 0;
	m_Capacity = 0;
	m_Offset.store(0, std::memory_order_relaxed);
	m_bInitialized = false;
}

void D3D12LinearAllocator::Reset() noexcept
{
	m_Offset.store(0, std::memory_order_release);
	m_HighWaterMark = 0;
}

D3D12LinearAllocation D3D12LinearAllocator::Allocate(uint64_t size, uint64_t alignment)
{
	assert(m_bInitialized && "D3D12LinearAllocator not initialized");
	assert(size > 0 && "Cannot allocate zero bytes");
	assert((alignment & (alignment - 1)) == 0 && "Alignment must be power of 2");

	const uint64_t alignedSize = AlignUp(size, alignment);

	uint64_t currentOffset;
	uint64_t alignedOffset;
	uint64_t newOffset;

	do
	{
		currentOffset = m_Offset.load(std::memory_order_acquire);
		alignedOffset = AlignUp(currentOffset, alignment);
		newOffset = alignedOffset + alignedSize;

		if (newOffset > m_Capacity)
		{
			throw std::bad_alloc();
		}
	} while (!m_Offset.compare_exchange_weak(currentOffset, newOffset, std::memory_order_acq_rel, std::memory_order_relaxed));

	uint64_t currentHigh = m_HighWaterMark.load(std::memory_order_relaxed);
	while (newOffset > currentHigh && !m_HighWaterMark.compare_exchange_weak(currentHigh, newOffset, std::memory_order_relaxed))
		;

	D3D12LinearAllocation alloc;
	alloc.CpuPtr = m_CpuBase + alignedOffset;
	alloc.GpuAddress = m_GpuBase + alignedOffset;
	alloc.Size = alignedSize;
	alloc.Offset = alignedOffset;

	return alloc;
}
