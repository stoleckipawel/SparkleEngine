#include "PCH.h"
#include "FrameGraph/FrameGraphResourceRegistry.h"

#include <cassert>

void FrameGraphResourceRegistry::Clear() noexcept
{
	m_metadataEntries.clear();
	m_registeredHandles.clear();
}

void FrameGraphResourceRegistry::RegisterBackBuffer(
    FrameGraphResourceHandle handle,
    const FrameGraphTextureDesc& desc,
    ResourceState initialState) noexcept
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
	metadata.accelerationStructureDesc = {};
}

void FrameGraphResourceRegistry::RegisterTransientTexture(
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
	metadata.accelerationStructureDesc = {};
}

void FrameGraphResourceRegistry::RegisterTransientBuffer(
    FrameGraphResourceHandle handle,
    const FrameGraphBufferDesc& desc,
    ResourceState initialState) noexcept
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
	metadata.accelerationStructureDesc = {};
}

void FrameGraphResourceRegistry::RegisterPersistentTexture(
    FrameGraphResourceHandle handle,
    const FrameGraphTextureDesc& desc,
    FrameGraphResourceKind kind,
    ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::Texture,
	    kind,
	    FrameGraphResourceOwnership::ExternalPersistent,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = desc;
	metadata.bufferDesc = {};
	metadata.accelerationStructureDesc = {};
}

void FrameGraphResourceRegistry::RegisterPersistentBuffer(
    FrameGraphResourceHandle handle,
    const FrameGraphBufferDesc& desc,
    ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::Buffer,
	    FrameGraphResourceKind::Buffer,
	    FrameGraphResourceOwnership::ExternalPersistent,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = {};
	metadata.bufferDesc = desc;
	metadata.accelerationStructureDesc = {};
}

void FrameGraphResourceRegistry::RegisterPersistentAccelerationStructure(
    FrameGraphResourceHandle handle,
    const FrameGraphAccelerationStructureDesc& desc,
    ResourceState initialState) noexcept
{
	FrameGraphResourceMetadata& metadata = RegisterMetadata(
	    handle,
	    FrameGraphResourceClass::AccelerationStructure,
	    FrameGraphResourceKind::AccelerationStructure,
	    FrameGraphResourceOwnership::ExternalPersistent,
	    desc.name,
	    initialState,
	    initialState);
	metadata.textureDesc = {};
	metadata.bufferDesc = {};
	metadata.accelerationStructureDesc = desc;
}

void FrameGraphResourceRegistry::SetBoundaryStates(
    FrameGraphResourceHandle handle,
    ResourceState initialState,
    ResourceState finalState) noexcept
{
	FrameGraphResourceMetadata& metadata = GetMetadata(handle);
	metadata.initialState = initialState;
	metadata.finalState = finalState;
}

bool FrameGraphResourceRegistry::IsRegistered(FrameGraphResourceHandle handle) const noexcept
{
	return handle.IsValid() && handle.index < m_metadataEntries.size() && m_metadataEntries[handle.index].handle == handle;
}

FrameGraphResourceMetadata& FrameGraphResourceRegistry::GetMetadata(FrameGraphResourceHandle handle) noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_metadataEntries[handle.index];
}

const FrameGraphResourceMetadata& FrameGraphResourceRegistry::GetMetadata(FrameGraphResourceHandle handle) const noexcept
{
	assert(IsRegistered(handle) && "FrameGraph resource handle is not registered.");
	return m_metadataEntries[handle.index];
}

void FrameGraphResourceRegistry::EnsureStorage(FrameGraphResourceHandle handle) noexcept
{
	assert(handle.IsValid());
	const std::size_t requiredSize = static_cast<std::size_t>(handle.index) + 1;
	if (m_metadataEntries.size() < requiredSize)
	{
		m_metadataEntries.resize(requiredSize);
	}
}

FrameGraphResourceMetadata& FrameGraphResourceRegistry::RegisterMetadata(
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

	if (!alreadyRegistered)
	{
		m_registeredHandles.push_back(handle);
	}

	return entry;
}
