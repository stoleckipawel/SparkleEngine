#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Window/Window.h"

#include <format>

static const auto g_frameGraphTextureLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

namespace FrameGraphTextureRegistration
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

	void FailMissingBackingResource(std::string_view operation, std::string_view resourceName, ResourceState initialState) noexcept
	{
		Diagnostics::Fail(
		    g_frameGraphTextureLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph external resource validation failed: operation='{}' resource='{}' initialState={} remediation='provide a valid native backing resource before importing it into the frame graph'",
		        operation,
		        resourceName.empty() ? "<unnamed>" : resourceName,
		        ResourceStateToString(initialState)));
	}

	std::string FormatBufferHandle(FrameGraphResourceHandle handle)
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
		Diagnostics::Fail(
		    g_frameGraphTextureLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph persistent-buffer validation failed: operation='{}' resource='{}' handle={} state={} hasResource={} remediation='bind persistent buffers through a valid frame-graph buffer handle with a native backing resource before declaring the pass resource use'",
		        operation,
		        resourceName.empty() ? "<unnamed>" : resourceName,
		        FormatBufferHandle(handle),
		        ResourceStateToString(state),
		        hasResource));
	}
}  // namespace FrameGraphTextureRegistration

FrameGraphTextureHandle FrameGraph::ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "BackBuffer");
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
		FrameGraphTextureRegistration::FailMissingBackingResource("ImportTexture", desc.name, initialState);
	}

	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "ImportedTexture");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedTexture(handle, resolvedDesc, FrameGraphTextureRegistration::ResolveTextureResourceKind(desc.kind), initialState);
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
		FrameGraphTextureRegistration::FailMissingBackingResource("ImportPersistentTexture", desc.name, initialState);
	}

	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "PersistentTexture");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentTexture(handle, resolvedDesc, FrameGraphTextureRegistration::ResolveTextureResourceKind(desc.kind), initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphTextureHandle{handle};
}

FrameGraphTextureHandle FrameGraph::CreateTexture(const FrameGraphTextureDesc& desc) noexcept
{
	const FrameGraphTextureDesc resolvedDesc = FrameGraphTextureRegistration::ResolveTextureDesc(desc, *m_window, "Texture");
	const FrameGraphResourceKind kind = FrameGraphTextureRegistration::ResolveTextureResourceKind(desc.kind);
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
		FrameGraphTextureRegistration::FailMissingBackingResource("ImportBuffer", desc.name, initialState);
	}

	const FrameGraphBufferDesc resolvedDesc = FrameGraphTextureRegistration::ResolveBufferDesc(desc, "ImportedBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterImportedBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphBufferHandle{handle};
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

FrameGraphBufferHandle FrameGraph::ReservePersistentBuffer(
    const FrameGraphBufferDesc& desc,
    ResourceState initialState) noexcept
{
	const FrameGraphBufferDesc resolvedDesc = FrameGraphTextureRegistration::ResolveBufferDesc(desc, "PersistentBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
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
		FrameGraphTextureRegistration::FailMissingBackingResource("ImportPersistentBuffer", desc.name, initialState);
	}

	const FrameGraphBufferDesc resolvedDesc = FrameGraphTextureRegistration::ResolveBufferDesc(desc, "PersistentBuffer");
	const FrameGraphResourceHandle handle = AllocateDynamicResourceHandle();
	m_resourceRegistry.RegisterPersistentBuffer(handle, resolvedDesc, initialState);
	m_resourceStateTracker.RegisterResource(handle, initialState);
	m_resourceStateTracker.UpdateCurrentState(handle, initialState);
	m_resourceResolver.RegisterResource(handle, resource);
	return FrameGraphBufferHandle{handle};
}

void FrameGraph::BindPersistentBuffer(
    FrameGraphBufferHandle handle,
    NativeResourceHandle resource,
    ResourceState currentState) noexcept
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

	m_resourceResolver.RegisterResource(resourceHandle, resource);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, currentState);
}

void FrameGraph::BindPersistentBuffer(
    FrameGraphBufferHandle handle,
    RhiOwnedResourceHandle resource,
    ResourceState currentState) noexcept
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

	BindPersistentBuffer(handle, m_renderHardwareInterface->GetResourceService().GetNativeResource(resource), currentState);
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

	m_resourceResolver.ClearResolvedAccess(resourceHandle);
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(resourceHandle);
	m_resourceStateTracker.UpdateCurrentState(resourceHandle, metadata.initialState);
}

FrameGraphResourceHandle FrameGraph::AllocateDynamicResourceHandle() noexcept
{
	const FrameGraphResourceHandle handle{m_nextDynamicResourceIndex};
	++m_nextDynamicResourceIndex;
	return handle;
}
