#include "PCH.h"
#include "FrameGraph/ResourceRegistry.h"

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
	for (const FrameGraphResourceHandle handle : m_registeredHandles)
	{
		FrameGraphResourceRuntimeState& runtimeState = GetRuntimeState(handle);
		runtimeState.currentState = GetMetadata(handle).initialState;
	}
}

void ResourceRegistry::RegisterBackBuffer(FrameGraphResourceHandle handle, const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::Texture,
	    FrameGraphResourceKind::BackBuffer,
	    FrameGraphResourceOwnership::Imported,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = desc;
	metadata.bufferDesc = {};
	GetRuntimeState(handle).currentState = initialState;
	ClearResolvedAccess(handle);
}

void ResourceRegistry::RegisterTransientTexture(
    FrameGraphResourceHandle handle,
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
    FrameGraphResourceHandle handle,
    const FrameGraphTextureDesc& desc,
    FrameGraphResourceKind kind,
    NativeResourceHandle resource,
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
	GetRuntimeState(handle).currentState = initialState;
	FrameGraphResourceAccess& access = GetResolvedAccess(handle);
	access = {};
	access.externalResource = resource;
}

void ResourceRegistry::RegisterTransientBuffer(FrameGraphResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
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
    FrameGraphResourceHandle handle,
    const FrameGraphBufferDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::Buffer,
	    FrameGraphResourceKind::Buffer,
	    FrameGraphResourceOwnership::Imported,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = {};
	metadata.bufferDesc = desc;
	GetRuntimeState(handle).currentState = initialState;
	FrameGraphResourceAccess& access = GetResolvedAccess(handle);
	access = {};
	access.externalResource = resource;
}

void ResourceRegistry::SetBoundaryStates(FrameGraphResourceHandle handle, ResourceState initialState, ResourceState finalState) noexcept
{
	FrameGraphResourceMetadata& metadata = GetMetadata(handle);
	metadata.initialState = initialState;
	metadata.finalState = finalState;
	GetRuntimeState(handle).currentState = initialState;
}

void ResourceRegistry::UpdateCurrentState(FrameGraphResourceHandle handle, ResourceState currentState) noexcept
{
	GetRuntimeState(handle).currentState = currentState;
}

void ResourceRegistry::ClearResolvedAccess(FrameGraphResourceHandle handle) noexcept
{
	GetResolvedAccess(handle) = {};
}

bool ResourceRegistry::IsRegistered(FrameGraphResourceHandle handle) const noexcept
{
	return handle.IsValid() && handle.index < m_metadataEntries.size() && m_metadataEntries[handle.index].handle == handle;
}

FrameGraphResourceMetadata& ResourceRegistry::GetMetadata(FrameGraphResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_metadataEntries[handle.index];
}

const FrameGraphResourceMetadata& ResourceRegistry::GetMetadata(FrameGraphResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_metadataEntries[handle.index];
}

FrameGraphResourceRuntimeState& ResourceRegistry::GetRuntimeState(FrameGraphResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_runtimeStates[handle.index];
}

const FrameGraphResourceRuntimeState& ResourceRegistry::GetRuntimeState(FrameGraphResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_runtimeStates[handle.index];
}

FrameGraphResourceAccess& ResourceRegistry::GetResolvedAccess(FrameGraphResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_resolvedAccessEntries[handle.index];
}

const FrameGraphResourceAccess& ResourceRegistry::GetResolvedAccess(FrameGraphResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_resolvedAccessEntries[handle.index];
}

void ResourceRegistry::EnsureStorage(FrameGraphResourceHandle handle) noexcept
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
    FrameGraphResourceHandle handle,
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
	if (!alreadyRegistered)
	{
		runtimeState.currentState = initialState;
	}

	if (!alreadyRegistered)
	{
		m_registeredHandles.push_back(handle);
	}

	return entry;
}