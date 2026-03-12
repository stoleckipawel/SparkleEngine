// ============================================================================
// D3D12DescriptorHandle.h
// ----------------------------------------------------------------------------
// Lightweight, type-aware descriptor identifier with CPU/GPU handles.
//
#pragma once

#include <d3d12.h>

class D3D12Rhi;

class D3D12DescriptorHandle
{
  public:
	explicit D3D12DescriptorHandle(
	    D3D12Rhi& rhi,
	    UINT idx,
	    D3D12_DESCRIPTOR_HEAP_TYPE type,
	    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE gpuStartHandle);

	// Default constructor creates an invalid handle (index ~0u, handles 0).
	// Useful for containers and default-constructible classes; must be assigned before use.
	D3D12DescriptorHandle() = default;

	UINT GetIndex() const noexcept { return m_index; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPU() const noexcept { return m_cpuHandle; }
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPU() const noexcept { return m_gpuHandle; }
	UINT GetIncrementSize() const noexcept { return m_incrementSize; }

	void SetIndex(UINT idx) noexcept { m_index = idx; }
	bool IsValid() const noexcept { return (m_index != InvalidIndex) && (m_cpuHandle.ptr != 0); }
	bool IsShaderVisible() const noexcept
	{
		return (m_heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) || (m_heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	static constexpr UINT InvalidIndex = ~0u;

  private:
	UINT m_index = InvalidIndex;  // Descriptor index within the heap (invalid by default)
	UINT m_incrementSize = 0;     // Cached descriptor increment size
	D3D12_DESCRIPTOR_HEAP_TYPE m_heapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle = {0};  // CPU handle for descriptor
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle = {0};  // GPU handle for descriptor (shader-visible only)
};
