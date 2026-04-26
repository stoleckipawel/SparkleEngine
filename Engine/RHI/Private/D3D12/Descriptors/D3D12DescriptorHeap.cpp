#include "PCH.h"
#include "D3D12/Descriptors/D3D12DescriptorHeap.h"

static const auto g_descriptorHeapLogger = Logging::GetOrCreateLogger("RHI.D3D12.Descriptors");

static constexpr UINT kRenderTargetDescriptorHeapSize = 4096;
static constexpr UINT kDepthStencilDescriptorHeapSize = 4096;

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Rhi& rhi, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, LPCWSTR name) :
    m_rhi(&rhi)
{
	m_desc.Type = type;
	m_desc.Flags = flags;
	m_desc.NumDescriptors = GetNumDescriptors();

	CHECK(m_rhi->GetDevice()->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(m_heap.ReleaseAndGetAddressOf())));
	m_heap->SetName(name);
}

D3D12DescriptorHeap::~D3D12DescriptorHeap() noexcept
{
	m_heap.Reset();
}

D3D12DescriptorHandle D3D12DescriptorHeap::GetHandleAt(UINT index) const
{
	if (index >= m_desc.NumDescriptors)
	{
		Diagnostics::Fail(g_descriptorHeapLogger, __FILE__, __LINE__, "Index out of range");
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
	switch (m_desc.Type)
	{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			return D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			return D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return kRenderTargetDescriptorHeapSize;
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return kDepthStencilDescriptorHeapSize;
		default:
			Diagnostics::Fail(g_descriptorHeapLogger, __FILE__, __LINE__, "Unsupported descriptor heap type.");
			return 0;
	}
}