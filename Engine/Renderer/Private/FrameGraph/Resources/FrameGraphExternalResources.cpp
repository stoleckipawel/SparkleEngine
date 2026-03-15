#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "Renderer/Public/CommandContext.h"

#include "D3D12DescriptorHeapManager.h"

#include <cassert>

namespace
{
	D3D12_SHADER_RESOURCE_VIEW_DESC BuildTextureShaderResourceViewDesc(const FrameGraphTextureDesc& desc) noexcept
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		return srvDesc;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC BuildBufferShaderResourceViewDesc(const FrameGraphBufferDesc& desc) noexcept
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (desc.strideInBytes > 0)
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.StructureByteStride = desc.strideInBytes;
			srvDesc.Buffer.NumElements = static_cast<UINT>(desc.sizeInBytes / desc.strideInBytes);
			return srvDesc;
		}

		assert(desc.sizeInBytes % sizeof(std::uint32_t) == 0);
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.NumElements = static_cast<UINT>(desc.sizeInBytes / sizeof(std::uint32_t));
		return srvDesc;
	}

}

void FrameGraph::SyncImportedResourceAccesses() const noexcept
{
	for (const ResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
		FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
		if (metadata.ownership != FrameGraphResourceOwnership::Imported)
		{
			continue;
		}

		if (access.externalResource == nullptr || m_descriptorHeapManager == nullptr || m_rhi == nullptr)
		{
			continue;
		}

		if (metadata.kind == FrameGraphResourceKind::ColorRenderTarget)
		{
			if (!access.renderTargetView.IsValid())
			{
				access.renderTargetView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			}

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = metadata.textureDesc.format;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			m_rhi->GetDevice()->CreateRenderTargetView(access.externalResource, &rtvDesc, access.renderTargetView.GetCPU());

			if (!access.shaderResourceView.IsValid())
			{
				access.shaderResourceView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}

			const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildTextureShaderResourceViewDesc(metadata.textureDesc);
			m_rhi->GetDevice()->CreateShaderResourceView(access.externalResource, &srvDesc, access.shaderResourceView.GetCPU());
		}
		else if (metadata.kind == FrameGraphResourceKind::DepthStencil)
		{
			if (!access.depthStencilView.IsValid())
			{
				access.depthStencilView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = metadata.textureDesc.format;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			m_rhi->GetDevice()->CreateDepthStencilView(access.externalResource, &dsvDesc, access.depthStencilView.GetCPU());
		}
		else if (metadata.kind == FrameGraphResourceKind::Buffer)
		{
			if (!access.shaderResourceView.IsValid())
			{
				access.shaderResourceView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}

			const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildBufferShaderResourceViewDesc(metadata.bufferDesc);
			m_rhi->GetDevice()->CreateShaderResourceView(access.externalResource, &srvDesc, access.shaderResourceView.GetCPU());
		}
	}
}

void FrameGraph::ReleaseExternalViewDescriptors() noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	for (const ResourceHandle handle : m_resourceRegistry.GetRegisteredHandles())
	{
		FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
		if (access.renderTargetView.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, access.renderTargetView);
			access.renderTargetView = {};
		}

		if (access.depthStencilView.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, access.depthStencilView);
			access.depthStencilView = {};
		}

		if (access.shaderResourceView.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, access.shaderResourceView);
			access.shaderResourceView = {};
		}
	}
}