#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "RHI/Public/Interop/ResourceState.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <string>
#include <string_view>
#include <vector>

enum class FrameGraphResourceClass : std::uint8_t
{
	Texture,
	Buffer
};

enum class FrameGraphResourceKind : std::uint8_t
{
	BackBuffer,
	DepthStencil,
	ColorRenderTarget,
	Buffer
};

enum class FrameGraphResourceOwnership : std::uint8_t
{
	Transient,
	Imported,
};

struct FrameGraphResourceAccess
{
	NativeResourceHandle externalResource = {};
	RhiCpuDescriptorHandle renderTargetView = {};
	RhiCpuDescriptorHandle depthStencilView = {};
	RhiCpuDescriptorHandle shaderResourceViewCpu = {};
	RhiGpuDescriptorHandle shaderResourceViewGpu = {};
	RhiCpuDescriptorHandle unorderedAccessViewCpu = {};
	RhiGpuDescriptorHandle unorderedAccessViewGpu = {};

	bool IsResolved() const noexcept { return static_cast<bool>(externalResource); }
};

struct FrameGraphResourceMetadata
{
	FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
	FrameGraphResourceClass resourceClass = FrameGraphResourceClass::Texture;
	FrameGraphResourceKind kind = FrameGraphResourceKind::BackBuffer;
	FrameGraphResourceOwnership ownership = FrameGraphResourceOwnership::Transient;
	ResourceState initialState = ResourceState::Common;
	ResourceState finalState = ResourceState::Common;
	std::string debugName;
	FrameGraphTextureDesc textureDesc{};
	FrameGraphBufferDesc bufferDesc{};
};

struct FrameGraphResourceRuntimeState
{
	ResourceState currentState = ResourceState::Common;
};

class ResourceRegistry final
{
  public:
	ResourceRegistry() = default;
	~ResourceRegistry() = default;

	ResourceRegistry(const ResourceRegistry&) = delete;
	ResourceRegistry& operator=(const ResourceRegistry&) = delete;
	ResourceRegistry(ResourceRegistry&&) = delete;
	ResourceRegistry& operator=(ResourceRegistry&&) = delete;

	void Clear() noexcept;
	void ResetCurrentStates() noexcept;
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
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
	void RegisterTransientBuffer(FrameGraphResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept;
	void RegisterImportedBuffer(
	    FrameGraphResourceHandle handle,
	    const FrameGraphBufferDesc& desc,
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
	void SetBoundaryStates(FrameGraphResourceHandle handle, ResourceState initialState, ResourceState finalState) noexcept;
	void UpdateCurrentState(FrameGraphResourceHandle handle, ResourceState currentState) noexcept;
	void ClearResolvedAccess(FrameGraphResourceHandle handle) noexcept;
	bool IsRegistered(FrameGraphResourceHandle handle) const noexcept;

	FrameGraphResourceMetadata& GetMetadata(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceMetadata& GetMetadata(FrameGraphResourceHandle handle) const noexcept;
	FrameGraphResourceRuntimeState& GetRuntimeState(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceRuntimeState& GetRuntimeState(FrameGraphResourceHandle handle) const noexcept;
	FrameGraphResourceAccess& GetResolvedAccess(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceAccess& GetResolvedAccess(FrameGraphResourceHandle handle) const noexcept;
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
	std::vector<FrameGraphResourceRuntimeState> m_runtimeStates;
	std::vector<FrameGraphResourceAccess> m_resolvedAccessEntries;
	std::vector<FrameGraphResourceHandle> m_registeredHandles;
};