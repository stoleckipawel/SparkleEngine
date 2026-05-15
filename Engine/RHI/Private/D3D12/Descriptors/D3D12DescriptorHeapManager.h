#pragma once

#include "D3D12/D3D12Rhi.h"
#include "Device/RenderHardwareInterface.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorAllocator.h"

class D3D12DescriptorHeapManager final
{
  public:
	explicit D3D12DescriptorHeapManager(D3D12Rhi& rhi);
	~D3D12DescriptorHeapManager() noexcept;

	D3D12DescriptorHeapManager(const D3D12DescriptorHeapManager&) = delete;
	D3D12DescriptorHeapManager& operator=(const D3D12DescriptorHeapManager&) = delete;
	D3D12DescriptorHeapManager(D3D12DescriptorHeapManager&&) = delete;
	D3D12DescriptorHeapManager& operator=(D3D12DescriptorHeapManager&&) = delete;

	void SetShaderVisibleHeaps(RenderCommandList& commandList) const;

	void AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
	{
		const D3D12DescriptorHandle handle = GetAllocator(type)->Allocate();
		cpuHandle = handle.GetCPU();
		gpuHandle = handle.GetGPU();
	}
	void FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	D3D12DescriptorHandle AllocateContiguous(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
	{
		return GetAllocator(type)->AllocateContiguous(count);
	}
	void FreeContiguous(
	    D3D12_DESCRIPTOR_HEAP_TYPE type,
	    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	    uint32_t count);
	void FreeContiguous(D3D12_DESCRIPTOR_HEAP_TYPE type, const D3D12DescriptorHandle& handle, uint32_t count)
	{
		GetAllocator(type)->FreeContiguous(handle, count);
	}

	D3D12DescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept;
	D3D12DescriptorAllocator* GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept;

  private:
	D3D12Rhi* m_rhi = nullptr;

	std::unique_ptr<D3D12DescriptorHeap> m_HeapSRV;
	std::unique_ptr<D3D12DescriptorAllocator> m_AllocatorSRV;

	std::unique_ptr<D3D12DescriptorHeap> m_HeapSampler;
	std::unique_ptr<D3D12DescriptorAllocator> m_AllocatorSampler;

	std::unique_ptr<D3D12DescriptorHeap> m_HeapDepthStencil;
	std::unique_ptr<D3D12DescriptorAllocator> m_AllocatorDepthStencil;

	std::unique_ptr<D3D12DescriptorHeap> m_HeapRenderTarget;
	std::unique_ptr<D3D12DescriptorAllocator> m_AllocatorRenderTarget;
};
