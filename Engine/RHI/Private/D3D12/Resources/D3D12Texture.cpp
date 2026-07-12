#include "PCH.h"
#include "D3D12/Resources/D3D12Texture.h"
#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"

#include <vector>

static const auto g_d3d12TextureLogger = Logging::GetOrCreateLogger("RHI.Textures");

static std::uint64_t D3D12TextureCalculatePayloadBytes(const RhiTextureUploadDesc& textureUpload) noexcept
{
	std::uint64_t byteCount = 0;
	for (const RhiTextureArraySliceUploadData& arraySlice : textureUpload.ArraySlices)
	{
		for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
		{
			byteCount += static_cast<std::uint64_t>(mipLevel.Data.size());
		}
	}
	return byteCount;
}

static const char* D3D12TextureFormatName(PixelFormat format) noexcept
{
	return PixelFormatName(format);
}

D3D12Texture::D3D12Texture(D3D12Rhi& rhi, RhiTextureUploadDesc textureUpload, D3D12DescriptorHeapManager& descriptorHeapManager) :
    m_rhi(rhi),
    m_textureUpload(std::move(textureUpload)),
    m_srvHandle(descriptorHeapManager.GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate()),
    m_descriptorHeapManager(&descriptorHeapManager)
{
	if (!m_srvHandle.IsValid())
	{
		Diagnostics::Fail(g_d3d12TextureLogger, __FILE__, __LINE__, "D3D12Texture: failed to allocate SRV descriptor.");
	}

	if (!m_textureUpload.IsValid())
	{
		Diagnostics::Fail(g_d3d12TextureLogger, __FILE__, __LINE__, "D3D12Texture: runtime texture upload payload is invalid.");
	}

	CreateResource();
	UploadToGPU();
	CreateShaderResourceView();
}

void D3D12Texture::CreateResource()
{
	m_texResourceDesc = {};
	m_texResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	m_texResourceDesc.Width = static_cast<UINT64>(m_textureUpload.Width);
	m_texResourceDesc.Height = static_cast<UINT>(m_textureUpload.Height);
	m_texResourceDesc.DepthOrArraySize = m_textureUpload.GetArraySize();
	m_texResourceDesc.MipLevels = m_textureUpload.GetMipCount();
	m_texResourceDesc.Format = D3D12TypeConversions::ToDxgiFormat(m_textureUpload.Format);
	m_texResourceDesc.SampleDesc.Count = 1;
	m_texResourceDesc.SampleDesc.Quality = 0;
	m_texResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	m_texResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_textureAllocation = m_rhi.GetMemoryAllocator().CreateTexture(
	    m_texResourceDesc,
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    nullptr,
	    RhiMemoryCategory::Texture,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"RHI_D3D12Texture");
	if (m_textureAllocation == nullptr || m_textureAllocation->Resource == nullptr)
	{
		Diagnostics::Fail(g_d3d12TextureLogger, __FILE__, __LINE__, "D3D12Texture: failed to allocate default texture resource.");
	}
	CHECK(m_textureAllocation->Resource.As(&m_textureResource));

	const UINT subresourceCount = static_cast<UINT>(m_textureUpload.GetSubresourceCount());
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textureResource.Get(), 0, subresourceCount);

	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	m_uploadAllocation = m_rhi.GetMemoryAllocator().CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    RhiMemoryCategory::Upload,
	    RhiMemoryResidencyClass::HostUpload,
	    L"RHI_D3D12TextureUpload");
	if (m_uploadAllocation == nullptr || m_uploadAllocation->Resource == nullptr)
	{
		Diagnostics::Fail(g_d3d12TextureLogger, __FILE__, __LINE__, "D3D12Texture: failed to allocate texture upload resource.");
	}
	CHECK(m_uploadAllocation->Resource.As(&m_uploadResource));
}

void D3D12Texture::UploadToGPU()
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	subresources.reserve(m_textureUpload.GetSubresourceCount());

	for (const RhiTextureArraySliceUploadData& arraySlice : m_textureUpload.ArraySlices)
	{
		for (const RhiTextureMipUploadData& mipLevel : arraySlice.MipLevels)
		{
			D3D12_SUBRESOURCE_DATA subresource = {};
			subresource.pData = mipLevel.Data.empty() ? nullptr : mipLevel.Data.data();
			subresource.RowPitch = static_cast<LONG_PTR>(mipLevel.RowPitch);
			subresource.SlicePitch = static_cast<LONG_PTR>(mipLevel.SlicePitch);
			subresources.push_back(subresource);
		}
	}

	UpdateSubresources(
	    m_rhi.GetCommandList(m_rhi.GetCurrentFrameIndex()).Get(),
	    m_textureResource.Get(),
	    m_uploadResource.Get(),
	    0,
	    0,
	    static_cast<UINT>(subresources.size()),
	    subresources.data());

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
	    m_textureResource.Get(),
	    D3D12_RESOURCE_STATE_COPY_DEST,
	    D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
	m_rhi.GetCommandList(m_rhi.GetCurrentFrameIndex())->ResourceBarrier(1, &barrier);
}

void D3D12Texture::CreateShaderResourceView()
{
	WriteShaderResourceView(RhiCpuDescriptorHandle{GetCPUHandle().ptr});
}

D3D12_SHADER_RESOURCE_VIEW_DESC D3D12Texture::BuildShaderResourceViewDesc() const noexcept
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = D3D12TypeConversions::ToDxgiFormat(m_textureUpload.Format);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (m_textureUpload.IsCube())
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = m_textureUpload.GetMipCount();
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = m_textureUpload.GetMipCount();
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}
	return srvDesc;
}

void D3D12Texture::WriteShaderResourceView(RhiCpuDescriptorHandle destination) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE nativeDestination{};
	nativeDestination.ptr = destination.Value;
	const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildShaderResourceViewDesc();
	m_rhi.GetDevice()->CreateShaderResourceView(m_textureResource.Get(), &srvDesc, nativeDestination);
}

TextureRuntimeInfo D3D12Texture::GetRuntimeInfo() const noexcept
{
	TextureRuntimeInfo info;
	info.Width = m_textureUpload.Width;
	info.Height = m_textureUpload.Height;
	info.ArraySize = m_textureUpload.GetArraySize();
	info.Dimension = m_textureUpload.Dimension;
	info.FormatName = D3D12TextureFormatName(m_textureUpload.Format);
	info.FormatIntent = m_textureUpload.FormatIntent;
	info.MipCount = m_textureUpload.GetMipCount();
	info.EstimatedByteSize = D3D12TextureCalculatePayloadBytes(m_textureUpload);
	info.GpuShaderResourceViewId = m_srvHandle.IsValid() ? m_srvHandle.GetGPU().ptr : 0;
	info.IsValid = m_textureUpload.IsValid() && m_textureResource != nullptr;
	return info;
}

D3D12Texture::~D3D12Texture() noexcept
{
	m_textureResource.Reset();
	m_uploadResource.Reset();
	m_textureAllocation.reset();
	m_uploadAllocation.reset();

	if (m_srvHandle.IsValid())
	{
		m_descriptorHeapManager->GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(m_srvHandle);
		m_srvHandle = D3D12DescriptorHandle();
	}
}
