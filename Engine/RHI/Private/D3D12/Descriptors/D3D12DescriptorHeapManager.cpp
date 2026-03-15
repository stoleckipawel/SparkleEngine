#include "PCH.h"
#include "D3D12DescriptorHeapManager.h"

#include "Renderer/Public/CommandContext.h"

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

D3D12DescriptorHeapManager::~D3D12DescriptorHeapManager() noexcept = default;

void D3D12DescriptorHeapManager::SetShaderVisibleHeaps(CommandContext& cmd) const
{
	ID3D12DescriptorHeap* heaps[] = {m_HeapSRV->GetRaw(), m_HeapSampler->GetRaw()};
	cmd.SetDescriptorHeaps(_countof(heaps), heaps);
}

void D3D12DescriptorHeapManager::FreeHandle(
	D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	(void)gpuHandle;

	D3D12DescriptorHeap* heap = GetHeap(type);
	if (heap == nullptr || m_rhi == nullptr)
	{
		return;
	}

	const D3D12DescriptorHandle firstHandle = heap->GetHandleAt(0);
	const UINT incrementSize = firstHandle.GetIncrementSize();
	if (incrementSize == 0 || cpuHandle.ptr < firstHandle.GetCPU().ptr)
	{
		return;
	}

	const SIZE_T byteOffset = cpuHandle.ptr - firstHandle.GetCPU().ptr;
	const UINT index = static_cast<UINT>(byteOffset / incrementSize);
	FreeHandle(type, heap->GetHandleAt(index));
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