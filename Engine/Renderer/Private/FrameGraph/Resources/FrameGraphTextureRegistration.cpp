#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

#include <format>

static const auto g_frameGraphTextureLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace FrameGraphTextureRegistration
{
	FrameGraphTextureDesc ResolveTextureDesc(const FrameGraphTextureDesc& desc, const Window& window, std::string_view defaultName)
	{
		const std::uint32_t width = desc.width > 0 ? desc.width : static_cast<std::uint32_t>(window.GetWidth());
		const std::uint32_t height = desc.height > 0 ? desc.height : static_cast<std::uint32_t>(window.GetHeight());
		const std::string debugName = desc.name.empty() ? std::string{defaultName} : desc.name;
		return FrameGraphTextureDesc{debugName, width, height, desc.format, desc.kind, desc.clearColor};
	}

	FrameGraphResourceKind ResolveTextureResourceKind(FrameGraphTextureKind kind) noexcept
	{
		return kind == FrameGraphTextureKind::DepthStencil ? FrameGraphResourceKind::DepthStencil
		                                                   : FrameGraphResourceKind::ColorRenderTarget;
	}

	FrameGraphBufferDesc ResolveBufferDesc(const FrameGraphBufferDesc& desc, std::string_view defaultName)
	{
		FrameGraphBufferDesc resolvedDesc = desc;
		if (resolvedDesc.name.empty())
		{
			resolvedDesc.name = std::string(defaultName);
		}

		return resolvedDesc;
	}

	std::string FormatResourceHandle(FrameGraphResourceHandle handle)
	{
		return handle.IsValid() ? std::format("{}", handle.index) : "invalid";
	}

	void FailInvalidPersistentBufferBinding(
	    std::string_view operation,
	    std::string_view resourceName,
	    FrameGraphResourceHandle handle,
	    ResourceState state,
	    bool hasResource) noexcept
	{
		Diagnostics::Fatal(
		    g_frameGraphTextureLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph persistent-buffer validation failed: operation='{}' resource='{}' handle={} state={} hasResource={} "
		        "remediation='bind persistent buffers through a valid frame-graph buffer handle with a native backing resource before "
		        "declaring the pass resource use'",
		        operation,
		        resourceName.empty() ? "<unnamed>" : resourceName,
		        FormatResourceHandle(handle),
		        ResourceStateToString(state),
		        hasResource));
	}

	void FailInvalidPersistentTextureBinding(
	    std::string_view operation,
	    std::string_view resourceName,
	    FrameGraphResourceHandle handle,
	    ResourceState state,
	    bool hasResource) noexcept
	{
		Diagnostics::Fatal(
		    g_frameGraphTextureLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph persistent-texture validation failed: operation='{}' resource='{}' handle={} state={} hasResource={} "
		        "remediation='bind persistent textures through a valid frame-graph texture handle with a native backing resource before "
		        "declaring the pass resource use'",
		        operation,
		        resourceName.empty() ? "<unnamed>" : resourceName,
		        FormatResourceHandle(handle),
		        ResourceStateToString(state),
		        hasResource));
	}
}  // namespace FrameGraphTextureRegistration

FrameGraphTextureHandle FrameGraph::ImportBackBuffer(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "BackBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterBackBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphTextureHandle{handle};
}

FrameGraphTextureHandle FrameGraph::ReservePersistentTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "PersistentTexture");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentTexture(
	    handle,
	    resolvedDesc,
	    FrameGraphTextureRegistration::ResolveTextureResourceKind(desc.kind),
	    initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphTextureHandle{handle};
}

FrameGraphTextureHandle FrameGraph::CreateTexture(const FrameGraphTextureDesc& desc) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "Texture");
	const FrameGraphResourceKind kind = FrameGraphTextureRegistration::ResolveTextureResourceKind(desc.kind);
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_virtualTransientResources.push_back(
	    VirtualTransientResource{.handle = handle, .resourceClass = FrameGraphResourceClass::Texture, .textureDesc = resolvedDesc});
	m_resourceRegistry.RegisterTransientTexture(handle, resolvedDesc, kind, ResourceState::Undefined);
	m_resourceStateTracker.RegisterResource(handle, ResourceState::Undefined);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphTextureHandle{handle};
}

FrameGraphBufferHandle FrameGraph::CreateBuffer(const FrameGraphBufferDesc& desc) noexcept
{
	const FrameGraphBufferDesc resolvedDesc = FrameGraphTextureRegistration::ResolveBufferDesc(desc, "Buffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_virtualTransientResources.push_back(
	    VirtualTransientResource{.handle = handle, .resourceClass = FrameGraphResourceClass::Buffer, .bufferDesc = resolvedDesc});
	m_resourceRegistry.RegisterTransientBuffer(handle, resolvedDesc, ResourceState::Common);
	m_resourceStateTracker.RegisterResource(handle, ResourceState::Common);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphBufferHandle{handle};
}

FrameGraphBufferHandle FrameGraph::ReservePersistentBuffer(const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept
{
	const FrameGraphBufferDesc resolvedDesc = FrameGraphTextureRegistration::ResolveBufferDesc(desc, "PersistentBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.ClearResolvedAccess(handle);
	return FrameGraphBufferHandle{handle};
}

void FrameGraph::BindPersistentBuffer(FrameGraphBufferHandle handle, RhiResourceHandle resource, ResourceState currentState) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	if (!resource)
	{
		FrameGraphTextureRegistration::FailInvalidPersistentBufferBinding(
		    "BindPersistentBuffer",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    false);
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		FrameGraphTextureRegistration::FailInvalidPersistentBufferBinding(
		    "BindPersistentBuffer",
		    {},
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource));
	}

	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	if (metadata.kind != FrameGraphResourceKind::Buffer || metadata.ownership != FrameGraphResourceOwnership::ExternalPersistent)
	{
		FrameGraphTextureRegistration::FailInvalidPersistentBufferBinding(
		    "BindPersistentBuffer",
		    metadata.debugName,
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource));
	}

	FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(resourceHandle);
	if (access.resource.Value != resource.Value)
	{
		ReleaseExternalResourceViews(resourceHandle);
		access.resource = resource;
	}
	m_resourceRegistry.SetExternalContentsProduced(resourceHandle, true);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, currentState);
}

void FrameGraph::BindPersistentTexture(FrameGraphTextureHandle handle, RhiResourceHandle resource, ResourceState currentState) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	if (!resource)
	{
		FrameGraphTextureRegistration::FailInvalidPersistentTextureBinding(
		    "BindPersistentTexture",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    false);
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		FrameGraphTextureRegistration::FailInvalidPersistentTextureBinding(
		    "BindPersistentTexture",
		    {},
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource));
	}

	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	if (metadata.resourceClass != FrameGraphResourceClass::Texture || metadata.ownership != FrameGraphResourceOwnership::ExternalPersistent)
	{
		FrameGraphTextureRegistration::FailInvalidPersistentTextureBinding(
		    "BindPersistentTexture",
		    metadata.debugName,
		    resourceHandle,
		    currentState,
		    static_cast<bool>(resource));
	}

	FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(resourceHandle);
	if (access.resource.Value != resource.Value)
	{
		ReleaseExternalResourceViews(resourceHandle);
		access.resource = resource;
	}
	m_resourceRegistry.SetExternalContentsProduced(resourceHandle, true);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, currentState);
}

void FrameGraph::BindPersistentTexture(FrameGraphTextureHandle handle, RhiOwnedResourceHandle resource, ResourceState currentState) noexcept
{
	if (m_renderHardwareInterface == nullptr || !resource)
	{
		FrameGraphTextureRegistration::FailInvalidPersistentTextureBinding(
		    "BindPersistentTexture",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    false);
	}

	BindPersistentTexture(handle, m_renderHardwareInterface->GetResourceService().GetResourceHandle(resource), currentState);
}

void FrameGraph::BindPersistentTexture(
    FrameGraphTextureHandle handle,
    RhiOwnedResourceHandle resource,
    RhiResourceViewHandle shaderResourceView,
    const FrameGraphTextureDesc& desc,
    ResourceState currentState) noexcept
{
	if (!handle.IsValid() || !resource || !shaderResourceView || m_renderHardwareInterface == nullptr)
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return;
	}

	FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	if (metadata.resourceClass != FrameGraphResourceClass::Texture || metadata.ownership != FrameGraphResourceOwnership::ExternalPersistent)
	{
		return;
	}
	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, metadata.debugName);
	if (metadata.textureDesc.width != resolvedDesc.width || metadata.textureDesc.height != resolvedDesc.height ||
	    metadata.textureDesc.format != resolvedDesc.format)
	{
		ReleaseExternalResourceViews(resourceHandle);
	}
	metadata.textureDesc = resolvedDesc;
	BindPersistentTexture(handle, resource, currentState);
	FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(resourceHandle);
	if (access.shaderResourceView != shaderResourceView)
	{
		if (access.shaderResourceView && access.ownsShaderResourceView)
		{
			m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(access.shaderResourceView);
		}
		access.shaderResourceView = shaderResourceView;
		access.ownsShaderResourceView = false;
	}
}

void FrameGraph::ClearPersistentTextureBinding(FrameGraphTextureHandle handle) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return;
	}

	ReleaseExternalResourceViews(resourceHandle);
	m_resourceResolver.ClearResolvedAccess(resourceHandle);
	m_resourceRegistry.SetExternalContentsProduced(resourceHandle, false);
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, metadata.initialState);
}

void FrameGraph::BindPersistentBuffer(FrameGraphBufferHandle handle, RhiOwnedResourceHandle resource, ResourceState currentState) noexcept
{
	if (m_renderHardwareInterface == nullptr || !resource)
	{
		FrameGraphTextureRegistration::FailInvalidPersistentBufferBinding(
		    "BindPersistentBuffer",
		    {},
		    handle.GetResourceHandle(),
		    currentState,
		    false);
	}

	BindPersistentBuffer(handle, m_renderHardwareInterface->GetResourceService().GetResourceHandle(resource), currentState);
}

void FrameGraph::BindPersistentBuffer(
    FrameGraphBufferHandle handle,
    RhiOwnedResourceHandle resource,
    const FrameGraphBufferDesc& desc,
    ResourceState currentState) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle.GetResourceHandle());
	const std::string resourceName = metadata.bufferDesc.name;
	const FrameGraphBufferDesc resolvedDesc = FrameGraphTextureRegistration::ResolveBufferDesc(desc, resourceName);
	if (metadata.bufferDesc.sizeInBytes != resolvedDesc.sizeInBytes || metadata.bufferDesc.strideInBytes != resolvedDesc.strideInBytes)
	{
		ReleaseExternalResourceViews(handle.GetResourceHandle());
	}
	metadata.bufferDesc = resolvedDesc;
	BindPersistentBuffer(handle, resource, currentState);
}

void FrameGraph::ClearPersistentBufferBinding(FrameGraphBufferHandle handle) noexcept
{
	if (!handle.IsValid())
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return;
	}

	ReleaseExternalResourceViews(resourceHandle);
	m_resourceResolver.ClearResolvedAccess(resourceHandle);
	m_resourceRegistry.SetExternalContentsProduced(resourceHandle, false);
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, metadata.initialState);
}

FrameGraphResourceHandle FrameGraph::AllocateDynamicResourceHandle() noexcept
{
	const FrameGraphResourceHandle handle{m_nextDynamicResourceIndex};
	++m_nextDynamicResourceIndex;
	return handle;
}
