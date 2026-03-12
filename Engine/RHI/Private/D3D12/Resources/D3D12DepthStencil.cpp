#include "PCH.h"
#include "D3D12DepthStencil.h"
#include "D3D12Rhi.h"
#include "Window.h"
#include "D3D12DescriptorHeapManager.h"
#include "DebugUtils.h"
#include "DepthConvention.h"

D3D12DepthStencil::D3D12DepthStencil(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager) :
    m_rhi(rhi),
    m_dsvHandle(descriptorHeapManager.AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)),
    m_window(&window),
    m_descriptorHeapManager(&descriptorHeapManager)
{
	CreateResource();
	CreateDepthStencilView();
}
void D3D12DepthStencil::CreateResource()
{
	m_depthStencilDesc.Format = RHISettings::DepthStencilFormat;
	m_depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = RHISettings::DepthStencilFormat;
	depthOptimizedClearValue.DepthStencil.Depth = DepthConvention::GetClearDepth();
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapDefaultProperties = {};
	heapDefaultProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDefaultProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapDefaultProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapDefaultProperties.CreationNodeMask = 0;
	heapDefaultProperties.VisibleNodeMask = 0;
	D3D12_RESOURCE_DESC depthStencilResourceDesc = {};
	depthStencilResourceDesc.Format = RHISettings::DepthStencilFormat;
	depthStencilResourceDesc.MipLevels = 1;
	depthStencilResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilResourceDesc.Height = static_cast<UINT>(m_window->GetHeight());
	depthStencilResourceDesc.Width = static_cast<UINT>(m_window->GetWidth());
	depthStencilResourceDesc.DepthOrArraySize = 1;
	depthStencilResourceDesc.SampleDesc.Count = 1;
	depthStencilResourceDesc.SampleDesc.Quality = 0;
	depthStencilResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthStencilResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	CHECK(m_rhi.GetDevice()->CreateCommittedResource(
	    &heapDefaultProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &depthStencilResourceDesc,
	    D3D12_RESOURCE_STATE_DEPTH_READ,
	    &depthOptimizedClearValue,
	    IID_PPV_ARGS(m_resource.ReleaseAndGetAddressOf())));
	DebugUtils::SetDebugName(m_resource, L"RHI_DepthStencil");
}

void D3D12DepthStencil::CreateDepthStencilView()
{
	m_rhi.GetDevice()->CreateDepthStencilView(m_resource.Get(), &m_depthStencilDesc, GetCPUHandle());
}

void D3D12DepthStencil::Clear() noexcept
{
	const float clearDepth = DepthConvention::GetClearDepth();
	m_rhi.GetCommandList()
	    ->ClearDepthStencilView(GetCPUHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clearDepth, 0, 0, nullptr);
}
void D3D12DepthStencil::SetWriteState() noexcept
{
	m_rhi.SetBarrier(m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void D3D12DepthStencil::SetReadState() noexcept
{
	m_rhi.SetBarrier(m_resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);
}

D3D12DepthStencil::~D3D12DepthStencil() noexcept
{
	m_resource.Reset();
	if (m_dsvHandle.IsValid())
	{
		m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, m_dsvHandle);
	}
}
