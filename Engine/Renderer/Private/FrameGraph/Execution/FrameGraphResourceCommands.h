#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

#include <span>

class FrameGraph;
class RenderCommandContext;

class FrameGraphResourceCommands final
{
  public:
	explicit FrameGraphResourceCommands(const FrameGraph& frameGraph) noexcept;

	void BindRenderTarget(
	    RenderCommandContext& cmd,
	    FrameGraphTextureHandle renderTargetHandle,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void BindRenderTargets(
	    RenderCommandContext& cmd,
	    std::span<const FrameGraphTextureHandle> renderTargetHandles,
	    FrameGraphTextureHandle depthStencilHandle = FrameGraphTextureHandle::Invalid()) const noexcept;
	void CopyTexture(RenderCommandContext& cmd, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle) const noexcept;
	void CopyBuffer(RenderCommandContext& cmd, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle) const noexcept;
	void ClearRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept;
	void ClearDepthStencil(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept;
	NativeResourceHandle ResolveResource(FrameGraphTextureHandle handle) const noexcept;
	NativeTextureViewInfo ResolveNativeTextureView(FrameGraphTextureHandle handle, ResourceState state) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept;
	RhiGpuDescriptorHandle ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept;
	RhiGpuVirtualAddress ResolveAccelerationStructureGpuAddress(FrameGraphAccelerationStructureHandle handle) const noexcept;

  private:
	const FrameGraph* m_frameGraph = nullptr;
};
