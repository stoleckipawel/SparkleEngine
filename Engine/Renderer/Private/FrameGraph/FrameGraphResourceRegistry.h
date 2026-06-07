#pragma once

#include "FrameGraph/FrameGraphResourceTypes.h"

#include <string_view>
#include <vector>

class FrameGraphResourceRegistry final
{
  public:
	FrameGraphResourceRegistry() = default;
	~FrameGraphResourceRegistry() = default;

	FrameGraphResourceRegistry(const FrameGraphResourceRegistry&) = delete;
	FrameGraphResourceRegistry& operator=(const FrameGraphResourceRegistry&) = delete;
	FrameGraphResourceRegistry(FrameGraphResourceRegistry&&) = delete;
	FrameGraphResourceRegistry& operator=(FrameGraphResourceRegistry&&) = delete;

	void Clear() noexcept;
	void RegisterBackBuffer(FrameGraphResourceHandle handle, const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	void RegisterTransientTexture(
	    FrameGraphResourceHandle handle,
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    ResourceState initialState) noexcept;
	void RegisterImportedTexture(
	    FrameGraphResourceHandle handle,
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    ResourceState initialState) noexcept;
	void RegisterTransientBuffer(FrameGraphResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept;
	void RegisterImportedBuffer(FrameGraphResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept;
	void RegisterPersistentTexture(
	    FrameGraphResourceHandle handle,
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    ResourceState initialState) noexcept;
	void RegisterPersistentBuffer(FrameGraphResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept;
	void RegisterImportedAccelerationStructure(
	    FrameGraphResourceHandle handle,
	    const FrameGraphAccelerationStructureDesc& desc,
	    ResourceState initialState) noexcept;
	void RegisterPersistentAccelerationStructure(
	    FrameGraphResourceHandle handle,
	    const FrameGraphAccelerationStructureDesc& desc,
	    ResourceState initialState) noexcept;
	void SetBoundaryStates(FrameGraphResourceHandle handle, ResourceState initialState, ResourceState finalState) noexcept;
	bool IsRegistered(FrameGraphResourceHandle handle) const noexcept;

	FrameGraphResourceMetadata& GetMetadata(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceMetadata& GetMetadata(FrameGraphResourceHandle handle) const noexcept;
	const std::vector<FrameGraphResourceHandle>& GetRegisteredHandles() const noexcept { return m_registeredHandles; }

  private:
	void EnsureStorage(FrameGraphResourceHandle handle) noexcept;
	FrameGraphResourceMetadata& RegisterMetadata(
	    FrameGraphResourceHandle handle,
	    FrameGraphResourceClass resourceClass,
	    FrameGraphResourceKind kind,
	    FrameGraphResourceOwnership ownership,
	    std::string_view debugName,
	    ResourceState initialState,
	    ResourceState finalState) noexcept;

	std::vector<FrameGraphResourceMetadata> m_metadataEntries;
	std::vector<FrameGraphResourceHandle> m_registeredHandles;
};
