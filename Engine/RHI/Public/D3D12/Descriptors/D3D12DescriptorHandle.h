// ============================================================================
// D3D12DescriptorHandle.h
// ----------------------------------------------------------------------------
// Lightweight, type-aware descriptor identifier with CPU/GPU handles.
//
// USAGE:
//   D3D12DescriptorHandle handle(index, heapType, cpuStart, gpuStart);
//   if (handle.IsValid()) {
//       auto cpu = handle.GetCPU();
//       auto gpu = handle.GetGPU();  // Only for shader-visible heaps
//   }
//
// DESIGN:
//   - Value type: cheap to copy and store in containers
//   - Handles are constructed from heap information, not raw pointers
//   - Invalid handles have index ~0u and null CPU handle
//
// NOTES:
//   - GPU handle is only valid for CBV_SRV_UAV and SAMPLER heap types
//   - Default constructor creates invalid handle for container use
// ============================================================================

#pragma once

#include <d3d12.h>

class D3D12Rhi;

class D3D12DescriptorHandle
{
  public:
	// Constructs a descriptor handle for a given heap type and index.
	// Parameters:
	//   rhi: reference to D3D12Rhi for device access
	//   idx: descriptor index within the heap
	//   type: D3D12 heap type (CBV_SRV_UAV, SAMPLER, RTV, DSV)
	//   cpuStartHandle/gpuStartHandle: start handles of the owning heap
	explicit D3D12DescriptorHandle(
	    D3D12Rhi& rhi,
	    UINT idx,
	    D3D12_DESCRIPTOR_HEAP_TYPE type,
	    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE gpuStartHandle);

	// Default constructor creates an invalid handle (index ~0u, handles 0).
	// Useful for containers and default-constructible classes; must be assigned before use.
	D3D12DescriptorHandle() = default;

	// Returns the descriptor index within the heap.
	UINT GetIndex() const noexcept { return m_index; }
	// Returns the CPU descriptor handle for binding or heap management.
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPU() const noexcept { return m_cpuHandle; }
	// Returns the GPU descriptor handle for shader-visible heaps.
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPU() const noexcept { return m_gpuHandle; }
	// Returns the device's descriptor increment size for this heap type.
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
