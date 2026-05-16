#include "PCH.h"

#include "D3D12/Resources/D3D12LinearAllocator.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include <cstring>

void D3D12LinearAllocator::Initialize(D3D12Rhi& rhi, uint64_t capacity, const wchar_t* debugName)
{
	assert(capacity > 0);

	m_rhi = &rhi;
	m_Capacity = capacity;
	m_Offset.store(0, std::memory_order_relaxed);

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

	m_Allocation = m_rhi->GetMemoryAllocator().CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::ConstantBuffer,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName);
	if (m_Allocation == nullptr || m_Allocation->Resource == nullptr)
	{
		throw std::runtime_error("D3D12LinearAllocator: Failed to create upload buffer");
	}
	m_Resource = m_Allocation->Resource;

	D3D12_RANGE readRange = {0, 0};
	const HRESULT hr = m_Resource->Map(0, &readRange, reinterpret_cast<void**>(&m_CpuBase));
	if (FAILED(hr))
	{
		m_Resource.Reset();
		m_Allocation.reset();
		throw std::runtime_error("D3D12LinearAllocator: Failed to map upload buffer");
	}
	m_Allocation->IsMapped = true;
	m_Allocation->CpuMappedAddress = m_CpuBase;

	m_GpuBase = m_Resource->GetGPUVirtualAddress();
	m_bInitialized = true;
}

void D3D12LinearAllocator::Shutdown()
{
	m_Resource.Reset();
	m_Allocation.reset();
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

	const uint64_t alignedSize = MathUtils::AlignUp(size, alignment);

	uint64_t currentOffset;
	uint64_t alignedOffset;
	uint64_t newOffset;

	do
	{
		currentOffset = m_Offset.load(std::memory_order_acquire);
		alignedOffset = MathUtils::AlignUp(currentOffset, alignment);
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
