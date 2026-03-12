#include "PCH.h"
#include "D3D12DescriptorHeap.h"
#include "DebugUtils.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Rhi& rhi, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, LPCWSTR name) :
    m_rhi(&rhi)
{
	m_desc.Type = type;
	m_desc.Flags = flags;
	m_desc.NumDescriptors = GetNumDescriptors();

	CHECK(m_rhi->GetDevice()->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(m_heap.ReleaseAndGetAddressOf())));
	DebugUtils::SetDebugName(m_heap, name);
}

D3D12DescriptorHeap::~D3D12DescriptorHeap() noexcept
{
	m_heap.Reset();
}

D3D12DescriptorHandle D3D12DescriptorHeap::GetHandleAt(UINT index) const
{
	if (index >= m_desc.NumDescriptors)
	{
		LOG_FATAL("Index out of range");
	}

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {0};
	if (m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
	}

	return D3D12DescriptorHandle(*m_rhi, index, m_desc.Type, m_heap->GetCPUDescriptorHandleForHeapStart(), gpuHandle);
}

UINT D3D12DescriptorHeap::GetNumDescriptors() const
{
	return m_desc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE
	                                                         : D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
}