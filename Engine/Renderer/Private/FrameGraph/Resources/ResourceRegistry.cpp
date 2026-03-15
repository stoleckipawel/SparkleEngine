#include "PCH.h"
#include "Renderer/Public/FrameGraph/ResourceRegistry.h"

#include "D3D12SwapChain.h"

#include <cassert>

void ResourceRegistry::Clear() noexcept
{
	m_metadataEntries.clear();
	m_runtimeStates.clear();
	m_resolvedAccessEntries.clear();
	m_registeredHandles.clear();
}

void ResourceRegistry::ResetCurrentStates() noexcept
{
	for (const ResourceHandle handle : m_registeredHandles)
	{
		FrameGraphResourceRuntimeState& runtimeState = GetRuntimeState(handle);
		runtimeState.currentState = GetMetadata(handle).initialState;
	}
}

void ResourceRegistry::RegisterBackBuffer(
    ResourceHandle handle,
    const FrameGraphTextureDesc& desc,
    D3D12SwapChain& swapChain,
    ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata =
	    RegisterMetadata(
	        handle,
	        FrameGraphResourceClass::Texture,
	        FrameGraphResourceKind::BackBuffer,
	        FrameGraphResourceOwnership::Imported,
	        desc.name,
	        initialState,
	        initialState);
	metadata.textureDesc = desc;
	metadata.bufferDesc = {};
	FrameGraphResourceAccess& access = GetResolvedAccess(handle);
	access = {};
	access.swapChain = &swapChain;
}

void ResourceRegistry::RegisterTransientTexture(
	ResourceHandle handle,
	const FrameGraphTextureDesc& desc,
	FrameGraphResourceKind kind,
	ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::Texture,
	    kind,
	    FrameGraphResourceOwnership::Transient,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = desc;
	metadata.bufferDesc = {};
	ClearResolvedAccess(handle);
}

void ResourceRegistry::RegisterImportedTexture(
    ResourceHandle handle,
    const FrameGraphTextureDesc& desc,
    FrameGraphResourceKind kind,
    ID3D12Resource& resource,
    ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::Texture,
	    kind,
	    FrameGraphResourceOwnership::Imported,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = desc;
	metadata.bufferDesc = {};
	FrameGraphResourceAccess& access = GetResolvedAccess(handle);
	access = {};
	access.externalResource = &resource;
}

void ResourceRegistry::RegisterTransientBuffer(ResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata =
	    RegisterMetadata(
	        handle,
	        FrameGraphResourceClass::Buffer,
	        FrameGraphResourceKind::Buffer,
	        FrameGraphResourceOwnership::Transient,
	        desc.name,
	        initialState,
	        initialState);
	metadata.textureDesc = {};
	metadata.bufferDesc = desc;
	ClearResolvedAccess(handle);
}

void ResourceRegistry::RegisterImportedBuffer(
	ResourceHandle handle,
	const FrameGraphBufferDesc& desc,
	ID3D12Resource& resource,
	ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata =
	    RegisterMetadata(
	        handle,
	        FrameGraphResourceClass::Buffer,
	        FrameGraphResourceKind::Buffer,
	        FrameGraphResourceOwnership::Imported,
	        desc.name,
	        initialState,
	        initialState);
	metadata.textureDesc = {};
	metadata.bufferDesc = desc;
	FrameGraphResourceAccess& access = GetResolvedAccess(handle);
	access = {};
	access.externalResource = &resource;
}

void ResourceRegistry::SetBoundaryStates(ResourceHandle handle, ResourceState initialState, ResourceState finalState) noexcept
{
	FrameGraphResourceMetadata& metadata = GetMetadata(handle);
	metadata.initialState = initialState;
	metadata.finalState = finalState;
	GetRuntimeState(handle).currentState = initialState;
}

void ResourceRegistry::ClearResolvedAccess(ResourceHandle handle) noexcept
{
	GetResolvedAccess(handle) = {};
}

bool ResourceRegistry::IsRegistered(ResourceHandle handle) const noexcept
{
	return handle.IsValid() && handle.index < m_metadataEntries.size() && m_metadataEntries[handle.index].handle == handle;
}

FrameGraphResourceMetadata& ResourceRegistry::GetMetadata(ResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_metadataEntries[handle.index];
}

const FrameGraphResourceMetadata& ResourceRegistry::GetMetadata(ResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_metadataEntries[handle.index];
}

FrameGraphResourceRuntimeState& ResourceRegistry::GetRuntimeState(ResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_runtimeStates[handle.index];
}

const FrameGraphResourceRuntimeState& ResourceRegistry::GetRuntimeState(ResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_runtimeStates[handle.index];
}

FrameGraphResourceAccess& ResourceRegistry::GetResolvedAccess(ResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_resolvedAccessEntries[handle.index];
}

const FrameGraphResourceAccess& ResourceRegistry::GetResolvedAccess(ResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_resolvedAccessEntries[handle.index];
}

void ResourceRegistry::EnsureStorage(ResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	const std::size_t requiredSize = static_cast<std::size_t>(handle.index) + 1;
	if (m_metadataEntries.size() < requiredSize)
	{
		m_metadataEntries.resize(requiredSize);
		m_runtimeStates.resize(requiredSize);
		m_resolvedAccessEntries.resize(requiredSize);
	}
}

FrameGraphResourceMetadata& ResourceRegistry::RegisterMetadata(
    ResourceHandle handle,
    FrameGraphResourceClass resourceClass,
    FrameGraphResourceKind kind,
	FrameGraphResourceOwnership ownership,
	std::string_view debugName,
	ResourceState initialState,
	ResourceState finalState) noexcept
{
	const bool alreadyRegistered = IsRegistered(handle);
	EnsureStorage(handle);
	FrameGraphResourceMetadata& entry = m_metadataEntries[handle.index];
	entry.handle = handle;
	entry.resourceClass = resourceClass;
	entry.kind = kind;
	entry.ownership = ownership;
	entry.initialState = initialState;
	entry.finalState = finalState;
	entry.debugName = std::string(debugName);

	FrameGraphResourceRuntimeState& runtimeState = m_runtimeStates[handle.index];
	runtimeState.currentState = initialState;

	if (!alreadyRegistered)
	{
		m_registeredHandles.push_back(handle);
	}

	return entry;
}