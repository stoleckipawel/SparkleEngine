#include "PCH.h"
#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "D3D12/Device/D3D12Rhi.h"

D3D12DescriptorHandle::D3D12DescriptorHandle(
    D3D12Rhi& rhi,
    UINT idx,
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStartHandle) :
    m_index(idx), m_descriptorType(type)
{
	m_incrementSize = rhi.GetDevice()->GetDescriptorHandleIncrementSize(m_descriptorType);

	m_cpuHandle.ptr = cpuStartHandle.ptr + static_cast<SIZE_T>(m_incrementSize) * static_cast<SIZE_T>(m_index);

	if (IsShaderVisible())
	{
		m_gpuHandle.ptr = gpuStartHandle.ptr + static_cast<SIZE_T>(m_incrementSize) * static_cast<SIZE_T>(m_index);
	}
}