#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "D3D12SwapChain.h"
#include "D3D12Texture.h"
#include "Window.h"

#include "Core/Public/Diagnostics/Log.h"

namespace
{
	FrameGraphTextureDesc ResolveTextureDesc(const FrameGraphTextureDesc& desc, const Window& window, std::string_view fallbackName)
	{
		const std::uint32_t width = desc.width > 0 ? desc.width : static_cast<std::uint32_t>(window.GetWidth());
		const std::uint32_t height = desc.height > 0 ? desc.height : static_cast<std::uint32_t>(window.GetHeight());
		const std::string debugName = desc.name.empty() ? std::string{fallbackName} : desc.name;
		return FrameGraphTextureDesc{debugName, width, height, desc.format, desc.kind};
	}

	FrameGraphResourceKind ResolveTextureResourceKind(FrameGraphTextureKind kind) noexcept
	{
		return kind == FrameGraphTextureKind::DepthStencil ? FrameGraphResourceKind::DepthStencil
		                                                  : FrameGraphResourceKind::ColorRenderTarget;
	}

	FrameGraphBufferDesc ResolveBufferDesc(const FrameGraphBufferDesc& desc, std::string_view fallbackName)
	{
		FrameGraphBufferDesc resolvedDesc = desc;
		if (resolvedDesc.name.empty())
		{
			resolvedDesc.name = std::string(fallbackName);
		}

		return resolvedDesc;
	}
}

TextureHandle FrameGraph::ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "BackBuffer");
	const ResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterBackBuffer(handle, resolvedDesc, *m_swapChain, initialState);
	return TextureHandle{handle};
}

TextureHandle FrameGraph::ImportTexture(const FrameGraphTextureDesc& desc, ID3D12Resource& resource, ResourceState initialState) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "ImportedTexture");
	const ResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedTexture(handle, resolvedDesc, ResolveTextureResourceKind(desc.kind), resource, initialState);
	return TextureHandle{handle};
}

TextureHandle FrameGraph::ImportTexture(const FrameGraphTextureDesc& desc, D3D12Texture& texture, ResourceState initialState) noexcept
{
	ID3D12Resource* resource = texture.GetResource().Get();
	if (resource == nullptr)
	{
		LOG_WARNING("FrameGraph::ImportTexture: imported texture has no backing resource.");
		return TextureHandle::Invalid();
	}

	return ImportTexture(desc, *resource, initialState);
}

TextureHandle FrameGraph::CreateTexture(const FrameGraphTextureDesc& desc) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "Texture");
	const FrameGraphResourceKind kind = ResolveTextureResourceKind(desc.kind);
	const ResourceHandle handle = AllocateDynamicResourceHandle();
	m_virtualTransientResources.push_back(
	    VirtualTransientResource{.handle = handle, .resourceClass = FrameGraphResourceClass::Texture, .textureDesc = resolvedDesc});
	m_resourceRegistry.RegisterTransientTexture(
	    handle,
	    resolvedDesc,
	    kind,
	    kind == FrameGraphResourceKind::DepthStencil ? ResourceState::DepthRead : ResourceState::Common);
	return TextureHandle{handle};
}

BufferHandle FrameGraph::ImportBuffer(const FrameGraphBufferDesc& desc, ID3D12Resource& resource, ResourceState initialState) noexcept
{
	const FrameGraphBufferDesc resolvedDesc = ResolveBufferDesc(desc, "ImportedBuffer");
	const ResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedBuffer(handle, resolvedDesc, resource, initialState);
	return BufferHandle{handle};
}

BufferHandle FrameGraph::CreateBuffer(const FrameGraphBufferDesc& desc) noexcept
{
	const FrameGraphBufferDesc resolvedDesc = ResolveBufferDesc(desc, "Buffer");
	const ResourceHandle handle = AllocateDynamicResourceHandle();
	m_virtualTransientResources.push_back(
	    VirtualTransientResource{.handle = handle, .resourceClass = FrameGraphResourceClass::Buffer, .bufferDesc = resolvedDesc});
	m_resourceRegistry.RegisterTransientBuffer(handle, resolvedDesc, ResourceState::Common);
	return BufferHandle{handle};
}

ResourceHandle FrameGraph::AllocateDynamicResourceHandle() noexcept
{
	const ResourceHandle handle{m_nextDynamicResourceIndex};
	++m_nextDynamicResourceIndex;
	return handle;
}