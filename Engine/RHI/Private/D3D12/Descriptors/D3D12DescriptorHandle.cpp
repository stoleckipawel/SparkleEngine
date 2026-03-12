#include "PCH.h"
#include "D3D12DescriptorHandle.h"
#include "D3D12Rhi.h"

D3D12DescriptorHandle::D3D12DescriptorHandle(
    D3D12Rhi& rhi,
    UINT idx,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStartHandle) :
    m_index(idx), m_heapType(type)
{
	// Cache the increment size at construction
	m_incrementSize = rhi.GetDevice()->GetDescriptorHandleIncrementSize(m_heapType);

	// Compute CPU handle: heap start + index * increment size
	m_cpuHandle.ptr = cpuStartHandle.ptr + static_cast<SIZE_T>(m_incrementSize) * static_cast<SIZE_T>(m_index);

	// Compute GPU handle only for shader-visible heaps
	if (IsShaderVisible())
	{
		m_gpuHandle.ptr = gpuStartHandle.ptr + static_cast<SIZE_T>(m_incrementSize) * static_cast<SIZE_T>(m_index);
	}
}