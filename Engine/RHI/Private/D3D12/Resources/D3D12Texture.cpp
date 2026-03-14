#include "PCH.h"
#include "D3D12Texture.h"
#include "D3D12Rhi.h"
#include "D3D12DescriptorHeapManager.h"
#include "Log.h"

#include <vector>

D3D12Texture::D3D12Texture(
	D3D12Rhi& rhi,
	TexturePayload texturePayload,
	D3D12DescriptorHeapManager& descriptorHeapManager) :
	m_rhi(rhi),
	m_texturePayload(std::move(texturePayload)),
	m_srvHandle(descriptorHeapManager.AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
	m_descriptorHeapManager(&descriptorHeapManager)
{
	if (!m_srvHandle.IsValid())
	{
		LOG_FATAL("D3D12Texture: failed to allocate SRV descriptor.");
	}

	if (!m_texturePayload.IsValid())
	{
		LOG_FATAL("D3D12Texture: runtime texture payload is invalid.");
	}

	CreateResource();
	UploadToGPU();
	CreateShaderResourceView();
}

void D3D12Texture::CreateResource()
{
	m_texResourceDesc = {};
	m_texResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	m_texResourceDesc.Width = static_cast<UINT64>(m_texturePayload.width);
	m_texResourceDesc.Height = static_cast<UINT>(m_texturePayload.height);
	m_texResourceDesc.DepthOrArraySize = 1;
	m_texResourceDesc.MipLevels = m_texturePayload.GetMipCount();
	m_texResourceDesc.Format = m_texturePayload.dxgiFormat;
	m_texResourceDesc.SampleDesc.Count = 1;
	m_texResourceDesc.SampleDesc.Quality = 0;
	m_texResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	m_texResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CD3DX12_HEAP_PROPERTIES heapDefaultProperties(D3D12_HEAP_TYPE_DEFAULT);
	CHECK(m_rhi.GetDevice()->CreateCommittedResource(
	    &heapDefaultProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &m_texResourceDesc,
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    nullptr,
	    IID_PPV_ARGS(m_textureResource.ReleaseAndGetAddressOf())));
	m_textureResource->SetName(L"RHI_D3D12Texture");

	const UINT subresourceCount = static_cast<UINT>(m_texturePayload.mipLevels.size());
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textureResource.Get(), 0, subresourceCount);

	CD3DX12_HEAP_PROPERTIES heapUploadProperties(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	CHECK(m_rhi.GetDevice()->CreateCommittedResource(
	    &heapUploadProperties,
	    D3D12_HEAP_FLAG_NONE,
	    &resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(m_uploadResource.ReleaseAndGetAddressOf())));
}

void D3D12Texture::UploadToGPU()
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	subresources.reserve(m_texturePayload.mipLevels.size());

	for (const auto& mipLevel : m_texturePayload.mipLevels)
	{
		D3D12_SUBRESOURCE_DATA subresource = {};
		subresource.pData = mipLevel.data.empty() ? nullptr : mipLevel.data.data();
		subresource.RowPitch = static_cast<LONG_PTR>(mipLevel.rowPitch);
		subresource.SlicePitch = static_cast<LONG_PTR>(mipLevel.slicePitch);
		subresources.push_back(subresource);
	}

	UpdateSubresources(
		m_rhi.GetCommandList().Get(),
		m_textureResource.Get(),
		m_uploadResource.Get(),
		0,
		0,
		static_cast<UINT>(subresources.size()),
		subresources.data());

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
	    m_textureResource.Get(),
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_rhi.GetCommandList()->ResourceBarrier(1, &barrier);
}

void D3D12Texture::CreateShaderResourceView()
{
	WriteShaderResourceView(GetCPUHandle());
}

D3D12_SHADER_RESOURCE_VIEW_DESC D3D12Texture::BuildShaderResourceViewDesc() const noexcept
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = m_texturePayload.dxgiFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_texturePayload.GetMipCount();
	return srvDesc;
}

void D3D12Texture::WriteShaderResourceView(D3D12_CPU_DESCRIPTOR_HANDLE destination) const
{
	const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildShaderResourceViewDesc();
	m_rhi.GetDevice()->CreateShaderResourceView(m_textureResource.Get(), &srvDesc, destination);
}

D3D12Texture::~D3D12Texture() noexcept
{
	m_textureResource.Reset();
	m_uploadResource.Reset();

	if (m_srvHandle.IsValid())
	{
		m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_srvHandle);
		m_srvHandle = D3D12DescriptorHandle();
	}
}
