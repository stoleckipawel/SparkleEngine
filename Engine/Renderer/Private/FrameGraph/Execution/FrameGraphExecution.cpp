#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "Renderer/Public/CommandContext.h"
#include "Renderer/Public/FrameContext.h"

#include <cassert>

void FrameGraph::Execute(const CompiledPlan& plan, CommandContext& cmd, const FrameContext& frame) const
{
	EnsureTransientResourcesMaterialized(plan);

	for (const PassIndex passIndex : plan.executionOrder)
	{
		const CompilePassRecord& passRecord = plan.passes[passIndex];
		EmitCompiledAliasingBarriers(cmd, passRecord.passName, passRecord.compiledAliasingBarriers);
		EmitCompiledBarriers(cmd, passRecord.passName, passRecord.compiledBarriers);
		m_passes[passIndex].executeCallback(*this, cmd, frame);
	}

	EmitCompiledAliasingBarriers(cmd, "FrameEnd", plan.finalAliasingBarriers);
	EmitCompiledBarriers(cmd, "FrameEnd", plan.finalBarriers);
}

void FrameGraph::BindRenderTarget(CommandContext& cmd, TextureHandle renderTargetHandle, TextureHandle depthStencilHandle) const noexcept
{
	const D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = ResolveRenderTargetView(renderTargetHandle.GetResourceHandle());
	if (!depthStencilHandle.IsValid())
	{
		cmd.SetRenderTarget(renderTargetView, nullptr);
		return;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = ResolveDepthStencilView(depthStencilHandle.GetResourceHandle());
	cmd.SetRenderTarget(renderTargetView, &depthStencilView);
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameGraph::ResolveShaderResourceView(TextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameGraph::ResolveShaderResourceView(BufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

void FrameGraph::CopyTexture(CommandContext& cmd, TextureHandle destinationHandle, TextureHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(cmd, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraph::CopyBuffer(CommandContext& cmd, BufferHandle destinationHandle, BufferHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(cmd, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraph::ClearRenderTarget(CommandContext& cmd, TextureHandle handle) const noexcept
{
	const ResourceHandle resourceHandle = handle.GetResourceHandle();
	const std::array<float, 4> clearColor = GetClearColor(resourceHandle);
	cmd.ClearRenderTarget(ResolveRenderTargetView(resourceHandle), clearColor.data());
}

void FrameGraph::ClearDepthStencil(CommandContext& cmd, TextureHandle handle) const noexcept
{
	const ResourceHandle resourceHandle = handle.GetResourceHandle();
	cmd.ClearDepthStencil(ResolveDepthStencilView(resourceHandle), GetClearDepth(resourceHandle));
}

void FrameGraph::CopyResource(CommandContext& cmd, ResourceHandle destinationHandle, ResourceHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());

	const FrameGraphResourceMetadata& destinationMetadata = m_resourceRegistry.GetMetadata(destinationHandle);
	const FrameGraphResourceMetadata& sourceMetadata = m_resourceRegistry.GetMetadata(sourceHandle);
	assert(destinationMetadata.resourceClass == sourceMetadata.resourceClass);
	assert(destinationMetadata.kind == sourceMetadata.kind ||
	       (destinationMetadata.resourceClass == FrameGraphResourceClass::Texture &&
	        sourceMetadata.resourceClass == FrameGraphResourceClass::Texture));

	ID3D12Resource* destinationResource = ResolveResource(destinationHandle);
	ID3D12Resource* sourceResource = ResolveResource(sourceHandle);
	assert(destinationResource != nullptr);
	assert(sourceResource != nullptr);
	cmd.CopyResource(destinationResource, sourceResource);
}
