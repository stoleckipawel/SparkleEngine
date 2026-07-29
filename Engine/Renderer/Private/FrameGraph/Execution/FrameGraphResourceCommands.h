#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"

#include <span>

class FrameGraph;
class RenderCommandContext;
struct NativeTextureViewInfo;
struct RhiNativeInteropRequest;

class FrameGraphResourceCommands final
{
  public:
	explicit FrameGraphResourceCommands(const FrameGraph& frameGraph) noexcept;

	void BindRenderTarget(
	    RenderCommandContext& commandContext,
	    FrameGraphTextureHandle renderTargetHandle,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void BindRenderTargets(
	    RenderCommandContext& commandContext,
	    std::span<const FrameGraphTextureHandle> renderTargetHandles,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void CopyTexture(RenderCommandContext& commandContext, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle) const noexcept;
	void CopyBuffer(RenderCommandContext& commandContext, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle) const noexcept;
	void ClearRenderTarget(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept;
	void ClearDepthStencil(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept;
	RhiResourceHandle ResolveResource(FrameGraphTextureHandle handle) const noexcept;
	NativeTextureViewInfo ResolveNativeTextureView(
	    FrameGraphTextureHandle handle,
	    ResourceState state,
	    const RhiNativeInteropRequest& request) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuVirtualAddress ResolveAccelerationStructureGpuAddress(FrameGraphAccelerationStructureHandle handle) const noexcept;

  private:
	const FrameGraph* m_frameGraph = nullptr;
};
