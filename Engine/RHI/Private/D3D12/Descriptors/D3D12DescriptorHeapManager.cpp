#include "PCH.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"

#include "D3D12/Commands/D3D12RenderCommandList.h"
#include "Resources/RhiResourceView.h"

D3D12DescriptorHeapManager::D3D12DescriptorHeapManager(D3D12Rhi& rhi) : m_rhi(&rhi)
{
	m_shaderResourceHeap = std::make_unique<D3D12DescriptorHeap>(
	    *m_rhi,
	    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	    L"CBVSRVUAVHeap");

	m_shaderResourceAllocator = std::make_unique<D3D12DescriptorAllocator>(m_shaderResourceHeap.get());
	m_resourceViewCopySourceHeap = std::make_unique<D3D12DescriptorHeap>(
	    *m_rhi,
	    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	    D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	    L"ResourceViewCopySourceHeap",
	    RhiResourceViewHandle::MaximumRecordCount);
	m_resourceViewCopySourceAllocator =
	    std::make_unique<D3D12DescriptorAllocator>(m_resourceViewCopySourceHeap.get());

	m_samplerHeap = std::make_unique<D3D12DescriptorHeap>(
	    *m_rhi,
	    D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
	    D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	    L"SamplerHeap");

	m_samplerAllocator = std::make_unique<D3D12DescriptorAllocator>(m_samplerHeap.get());

	m_depthStencilHeap =
	    std::make_unique<D3D12DescriptorHeap>(*m_rhi, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, L"DepthStencilHeap");

	m_depthStencilAllocator = std::make_unique<D3D12DescriptorAllocator>(m_depthStencilHeap.get());

	m_renderTargetHeap =
	    std::make_unique<D3D12DescriptorHeap>(*m_rhi, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, L"RenderTargetHeap");

	m_renderTargetAllocator = std::make_unique<D3D12DescriptorAllocator>(m_renderTargetHeap.get());
}

D3D12DescriptorHeapManager::~D3D12DescriptorHeapManager() noexcept = default;

void D3D12DescriptorHeapManager::AllocateHandle(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
{
	const D3D12DescriptorHandle handle = GetAllocator(type)->Allocate();
	cpuHandle = handle.GetCPU();
	gpuHandle = handle.GetGPU();
}

D3D12DescriptorHandle D3D12DescriptorHeapManager::AllocateResourceViewCopySource()
{
	return m_resourceViewCopySourceAllocator->Allocate();
}

void D3D12DescriptorHeapManager::FreeResourceViewCopySource(const D3D12DescriptorHandle& handle) noexcept
{
	m_resourceViewCopySourceAllocator->Free(handle);
}

D3D12DescriptorHandle D3D12DescriptorHeapManager::AllocateContiguous(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    uint32_t count)
{
	return GetAllocator(type)->AllocateContiguous(count);
}

void D3D12DescriptorHeapManager::BindGlobalDescriptorState(D3D12RenderCommandList& commandList) const
{
	ID3D12DescriptorHeap* const heaps[] = {m_shaderResourceHeap->GetRaw(), m_samplerHeap->GetRaw()};
	commandList.SetShaderVisibleDescriptorHeaps(static_cast<std::uint32_t>(_countof(heaps)), heaps);
}

void D3D12DescriptorHeapManager::FreeHandle(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	(void) gpuHandle;

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
	GetAllocator(type)->Free(heap->GetHandleAt(index));
}

void D3D12DescriptorHeapManager::FreeContiguous(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle,
    uint32_t count)
{
	(void) gpuHandle;

	if (count == 0)
	{
		return;
	}

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
	if ((byteOffset % incrementSize) != 0)
	{
		return;
	}

	const UINT index = static_cast<UINT>(byteOffset / incrementSize);
	FreeContiguous(type, heap->GetHandleAt(index), count);
}

void D3D12DescriptorHeapManager::FreeContiguous(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    const D3D12DescriptorHandle& handle,
    uint32_t count)
{
	GetAllocator(type)->FreeContiguous(handle, count);
}

D3D12DescriptorHeap* D3D12DescriptorHeapManager::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
{
	switch (type)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return m_shaderResourceHeap.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return m_samplerHeap.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return m_renderTargetHeap.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return m_depthStencilHeap.get();
		default:
			return nullptr;
	}
}

D3D12DescriptorAllocator* D3D12DescriptorHeapManager::GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
{
	switch (type)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return m_shaderResourceAllocator.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return m_samplerAllocator.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return m_renderTargetAllocator.get();
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return m_depthStencilAllocator.get();
		default:
			return nullptr;
	}
}
