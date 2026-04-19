#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/ResourceHandle.h"
#include "RHI/Public/Interop/ResourceState.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"

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
	ResourceHandle handle = ResourceHandle::Invalid();
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

class SPARKLE_RENDERER_API ResourceRegistry final
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
	void RegisterBackBuffer(ResourceHandle handle, const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept;
	void RegisterTransientTexture(
	    ResourceHandle handle,
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    ResourceState initialState) noexcept;
	void RegisterImportedTexture(
	    ResourceHandle handle,
	    const FrameGraphTextureDesc& desc,
	    FrameGraphResourceKind kind,
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
	void RegisterTransientBuffer(ResourceHandle handle, const FrameGraphBufferDesc& desc, ResourceState initialState) noexcept;
	void RegisterImportedBuffer(
	    ResourceHandle handle,
	    const FrameGraphBufferDesc& desc,
	    NativeResourceHandle resource,
	    ResourceState initialState) noexcept;
	void SetBoundaryStates(ResourceHandle handle, ResourceState initialState, ResourceState finalState) noexcept;
	void UpdateCurrentState(ResourceHandle handle, ResourceState currentState) noexcept;
	void ClearResolvedAccess(ResourceHandle handle) noexcept;
	bool IsRegistered(ResourceHandle handle) const noexcept;

	FrameGraphResourceMetadata& GetMetadata(ResourceHandle handle) noexcept;
	const FrameGraphResourceMetadata& GetMetadata(ResourceHandle handle) const noexcept;
	FrameGraphResourceRuntimeState& GetRuntimeState(ResourceHandle handle) noexcept;
	const FrameGraphResourceRuntimeState& GetRuntimeState(ResourceHandle handle) const noexcept;
	FrameGraphResourceAccess& GetResolvedAccess(ResourceHandle handle) noexcept;
	const FrameGraphResourceAccess& GetResolvedAccess(ResourceHandle handle) const noexcept;
	const std::vector<ResourceHandle>& GetRegisteredHandles() const noexcept { return m_registeredHandles; }

  private:
	void EnsureStorage(ResourceHandle handle) noexcept;
	FrameGraphResourceMetadata& RegisterMetadata(
	    ResourceHandle handle,
	    FrameGraphResourceClass resourceClass,
	    FrameGraphResourceKind kind,
	    FrameGraphResourceOwnership ownership,
	    std::string_view debugName,
	    ResourceState initialState,
	    ResourceState finalState) noexcept;

	std::vector<FrameGraphResourceMetadata> m_metadataEntries;
	std::vector<FrameGraphResourceRuntimeState> m_runtimeStates;
	std::vector<FrameGraphResourceAccess> m_resolvedAccessEntries;
	std::vector<ResourceHandle> m_registeredHandles;
};