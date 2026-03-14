#include "PCH.h"
#include "D3D12DescriptorHeapManager.h"

D3D12DescriptorHeapManager::D3D12DescriptorHeapManager(D3D12Rhi& rhi) : m_rhi(&rhi)
{
	m_HeapSRV = std::make_unique<D3D12DescriptorHeap>(
	    *m_rhi,
	    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	    L"CBVSRVUAVHeap");

	m_AllocatorSRV = std::make_unique<D3D12DescriptorAllocator>(m_HeapSRV.get());

	m_HeapSampler = std::make_unique<D3D12DescriptorHeap>(
	    *m_rhi,
	    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
	    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	    L"SamplerHeap");

	m_AllocatorSampler = std::make_unique<D3D12DescriptorAllocator>(m_HeapSampler.get());

	m_HeapDepthStencil =
	    std::make_unique<D3D12DescriptorHeap>(*m_rhi, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, L"DepthStencilHeap");

	m_AllocatorDepthStencil = std::make_unique<D3D12DescriptorAllocator>(m_HeapDepthStencil.get());

	m_HeapRenderTarget =
	    std::make_unique<D3D12DescriptorHeap>(*m_rhi, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, L"RenderTargetHeap");

	m_AllocatorRenderTarget = std::make_unique<D3D12DescriptorAllocator>(m_HeapRenderTarget.get());
}

void D3D12DescriptorHeapManager::SetShaderVisibleHeaps() const
{
	ID3D12DescriptorHeap* heaps[] = {m_HeapSRV->GetRaw(), m_HeapSampler->GetRaw()};

	m_rhi->GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);
}

void D3D12DescriptorHeapManager::AllocateHandle(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE& outCPU,
    D3D12_GPU_DESCRIPTOR_HANDLE& outGPU)
{
	D3D12DescriptorAllocator* allocator = GetAllocator(type);
	const D3D12DescriptorHandle handle = allocator->Allocate();
	outCPU = handle.GetCPU();
	outGPU = handle.GetGPU();
}

void D3D12DescriptorHeapManager::FreeHandle(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	D3D12DescriptorHeap* heap = GetHeap(type);
	D3D12DescriptorAllocator* allocator = GetAllocator(type);

	if (!heap || !allocator)
	{
		LOG_FATAL("FreeHandle: invalid heap or allocator");
	}

	const auto heapCPUStart = heap->GetRaw()->GetCPUDescriptorHandleForHeapStart();
	const UINT increment = m_rhi->GetDevice()->GetDescriptorHandleIncrementSize(type);
	const SIZE_T delta = (cpuHandle.ptr - heapCPUStart.ptr);
	const UINT index = static_cast<UINT>(delta / static_cast<SIZE_T>(increment));

	D3D12_GPU_DESCRIPTOR_HANDLE heapGPUStart = {0};
	const auto heapDesc = heap->GetRaw()->GetDesc();
	if (heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		heapGPUStart = heap->GetRaw()->GetGPUDescriptorHandleForHeapStart();
	}

	const D3D12DescriptorHandle handle(*m_rhi, index, type, heapCPUStart, heapGPUStart);
	allocator->Free(handle);
}

D3D12DescriptorHeap* D3D12DescriptorHeapManager::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
{
	switch (type)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return m_HeapSRV.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return m_HeapSampler.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return m_HeapRenderTarget.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return m_HeapDepthStencil.get();
		default:
			return nullptr;
	}
}

D3D12DescriptorAllocator* D3D12DescriptorHeapManager::GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
{
	switch (type)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return m_AllocatorSRV.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return m_AllocatorSampler.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return m_AllocatorRenderTarget.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return m_AllocatorDepthStencil.get();
		default:
			return nullptr;
	}
}

D3D12DescriptorHeapManager::~D3D12DescriptorHeapManager() noexcept
{
	m_AllocatorRenderTarget.reset();
	m_HeapRenderTarget.reset();

	m_AllocatorDepthStencil.reset();
	m_HeapDepthStencil.reset();

	m_AllocatorSampler.reset();
	m_HeapSampler.reset();

	m_AllocatorSRV.reset();
	m_HeapSRV.reset();
}