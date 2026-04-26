#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "GPU/CommandContext.h"
#include "GPU/FrameExecutionDiagnostics.h"
#include "GPU/PassExecutionDiagnostics.h"
#include "Frame/FrameContext.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <cassert>
#include <string>

void FrameGraph::Execute(
    const CompiledPlan& plan,
    CommandContext& cmd,
    const FrameContext& frame,
	const RenderPassContext& renderPassContext,
	FrameExecutionDiagnostics& frameDiagnostics) const
{
	cmd.EnableDrawDispatchDiagnostics();

	static constexpr auto kFrameGraphExecuteName = Diagnostics::DiagnosticName{"Renderer.FrameGraph.Execute"};
	SPARKLE_CPU_SCOPE(kFrameGraphExecuteName);

	EnsureTransientResourcesMaterialized(plan);

	for (const PassIndex passIndex : plan.executionOrder)
	{
		const CompilePassRecord& passRecord = plan.passes[passIndex];
		if (!passRecord.compiledAliasingBarriers.empty())
		{
			std::string aliasBarrierMarker = passRecord.diagnosticName;
			aliasBarrierMarker += ".AliasingBarriers";
			frameDiagnostics.InsertGpuMarker(cmd, aliasBarrierMarker);
		}
		EmitCompiledAliasingBarriers(cmd, passRecord.passName, passRecord.compiledAliasingBarriers);
		if (!passRecord.compiledBarriers.empty())
		{
			std::string barrierMarker = passRecord.diagnosticName;
			barrierMarker += ".ResourceBarriers";
			frameDiagnostics.InsertGpuMarker(cmd, barrierMarker);
		}
		EmitCompiledBarriers(cmd, passRecord.passName, passRecord.compiledBarriers);
		PassExecutionDiagnostics passDiagnostics(
		    frameDiagnostics,
		    cmd,
		    passRecord.diagnosticName,
		    passRecord.displayLabel,
		    passRecord.eventScopeLabel,
		    passRecord.passKind);
		auto passScope = passDiagnostics.BeginPassGpuEvent();
		auto passTimer = passDiagnostics.BeginPassTimer();
		RenderGraphPassContext passContext{cmd, frame, renderPassContext, passDiagnostics, *this};
		m_passes[passIndex].executeCallback(passContext);
	}

	if (!plan.finalAliasingBarriers.empty())
	{
		frameDiagnostics.InsertGpuMarker(cmd, "Renderer.FrameGraph.FrameEnd.AliasingBarriers");
	}
	EmitCompiledAliasingBarriers(cmd, "FrameEnd", plan.finalAliasingBarriers);
	if (!plan.finalBarriers.empty())
	{
		frameDiagnostics.InsertGpuMarker(cmd, "Renderer.FrameGraph.FrameEnd.ResourceBarriers");
	}
	EmitCompiledBarriers(cmd, "FrameEnd", plan.finalBarriers);
}

void FrameGraph::BindRenderTarget(CommandContext& cmd, TextureHandle renderTargetHandle, TextureHandle depthStencilHandle) const noexcept
{
	const RhiCpuDescriptorHandle renderTargetView = ResolveRenderTargetView(renderTargetHandle.GetResourceHandle());
	if (!depthStencilHandle.IsValid())
	{
		cmd.SetRenderTarget(renderTargetView, nullptr);
		return;
	}

	const RhiCpuDescriptorHandle depthStencilView = ResolveDepthStencilView(depthStencilHandle.GetResourceHandle());
	cmd.SetRenderTarget(renderTargetView, &depthStencilView);
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(TextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(BufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(TextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(BufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
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

NativeResourceHandle FrameGraph::ResolveResource(TextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveResource(handle.GetResourceHandle());
}

void FrameGraph::CopyResource(CommandContext& cmd, ResourceHandle destinationHandle, ResourceHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());

	const FrameGraphResourceMetadata& destinationMetadata = m_resourceRegistry.GetMetadata(destinationHandle);
	const FrameGraphResourceMetadata& sourceMetadata = m_resourceRegistry.GetMetadata(sourceHandle);
	assert(destinationMetadata.resourceClass == sourceMetadata.resourceClass);
	assert(
	    destinationMetadata.kind == sourceMetadata.kind || (destinationMetadata.resourceClass == FrameGraphResourceClass::Texture &&
	                                                        sourceMetadata.resourceClass == FrameGraphResourceClass::Texture));

	const NativeResourceHandle destinationResource = ResolveResource(destinationHandle);
	const NativeResourceHandle sourceResource = ResolveResource(sourceHandle);
	assert(destinationResource);
	assert(sourceResource);
	cmd.CopyResource(destinationResource, sourceResource);
}
