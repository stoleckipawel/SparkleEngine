#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Window/Window.h"

static const auto g_frameGraphTextureLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace
{
	FrameGraphTextureDesc ResolveTextureDesc(const FrameGraphTextureDesc& desc, const Window& window, std::string_view fallbackName)
	{
		const std::uint32_t width = desc.width > 0 ? desc.width : static_cast<std::uint32_t>(window.GetWidth());
		const std::uint32_t height = desc.height > 0 ? desc.height : static_cast<std::uint32_t>(window.GetHeight());
		const std::string debugName = desc.name.empty() ? std::string{fallbackName} : desc.name;
		return FrameGraphTextureDesc{debugName, width, height, desc.format, desc.kind, desc.clearColor};
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
}  // namespace

FrameGraphTextureHandle FrameGraph::ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "BackBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterBackBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphTextureHandle{handle};
}

FrameGraphTextureHandle FrameGraph::ImportTexture(
    const FrameGraphTextureDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	if (!resource)
	{
		SPDLOG_LOGGER_WARN(g_frameGraphTextureLogger, "FrameGraph::ImportTexture: imported texture has no backing resource.");
		return FrameGraphTextureHandle::Invalid();
	}

	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "ImportedTexture");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedTexture(handle, resolvedDesc, ResolveTextureResourceKind(desc.kind), initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphTextureHandle{handle};
}

FrameGraphTextureHandle FrameGraph::ImportPersistentTexture(
    const FrameGraphTextureDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	if (!resource)
	{
		SPDLOG_LOGGER_WARN(g_frameGraphTextureLogger, "FrameGraph::ImportPersistentTexture: persistent texture has no backing resource.");
		return FrameGraphTextureHandle::Invalid();
	}

	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "PersistentTexture");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentTexture(handle, resolvedDesc, ResolveTextureResourceKind(desc.kind), initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphTextureHandle{handle};
}

FrameGraphTextureHandle FrameGraph::CreateTexture(const FrameGraphTextureDesc& desc) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = ResolveTextureDesc(desc, *m_window, "Texture");
	const FrameGraphResourceKind kind = ResolveTextureResourceKind(desc.kind);
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_virtualTransientResources.push_back(
	    VirtualTransientResource{.handle = handle, .resourceClass = FrameGraphResourceClass::Texture, .textureDesc = resolvedDesc});
	m_resourceRegistry.RegisterTransientTexture(
	    handle,
	    resolvedDesc,
	    kind,
	    ResourceState::Undefined);
	m_resourceStateTracker.RegisterResource(handle, ResourceState::Undefined);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphTextureHandle{handle};
}

FrameGraphBufferHandle FrameGraph::ImportBuffer(const FrameGraphBufferDesc& desc, NativeResourceHandle resource, ResourceState initialState) noexcept
{
	if (!resource)
	{
		SPDLOG_LOGGER_WARN(g_frameGraphTextureLogger, "FrameGraph::ImportBuffer: imported buffer has no backing resource.");
		return FrameGraphBufferHandle::Invalid();
	}

	const FrameGraphBufferDesc resolvedDesc = ResolveBufferDesc(desc, "ImportedBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphBufferHandle{handle};
}

FrameGraphBufferHandle FrameGraph::CreateBuffer(const FrameGraphBufferDesc& desc) noexcept
{
	const FrameGraphBufferDesc resolvedDesc = ResolveBufferDesc(desc, "Buffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_virtualTransientResources.push_back(
	    VirtualTransientResource{.handle = handle, .resourceClass = FrameGraphResourceClass::Buffer, .bufferDesc = resolvedDesc});
	m_resourceRegistry.RegisterTransientBuffer(handle, resolvedDesc, ResourceState::Common);
	m_resourceStateTracker.RegisterResource(handle, ResourceState::Common);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphBufferHandle{handle};
}

FrameGraphBufferHandle FrameGraph::ImportPersistentBuffer(
    const FrameGraphBufferDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	if (!resource)
	{
		SPDLOG_LOGGER_WARN(g_frameGraphTextureLogger, "FrameGraph::ImportPersistentBuffer: persistent buffer has no backing resource.");
		return FrameGraphBufferHandle::Invalid();
	}

	const FrameGraphBufferDesc resolvedDesc = ResolveBufferDesc(desc, "PersistentBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphBufferHandle{handle};
}

FrameGraphResourceHandle FrameGraph::AllocateDynamicResourceHandle() noexcept
{
	const FrameGraphResourceHandle handle{m_nextDynamicResourceIndex};
	++m_nextDynamicResourceIndex;
	return handle;
}
