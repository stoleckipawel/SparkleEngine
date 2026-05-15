#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/FrameContext.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <array>
#include <cassert>
#include <string>

struct FrameGraphFramebuffer final
{
	std::array<RhiCpuDescriptorHandle, 8> renderTargetViews = {};
	std::uint32_t renderTargetCount = 0;
	RhiCpuDescriptorHandle depthStencilView = {};

	bool HasDepthStencil() const noexcept { return static_cast<bool>(depthStencilView); }
};

void FrameGraph::Execute(
    const CompiledPlan& plan,
    RenderCommandContext& cmd,
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

void FrameGraph::BindRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle renderTargetHandle, FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	FrameGraphFramebuffer framebuffer{};
	framebuffer.renderTargetViews[0] = ResolveRenderTargetView(renderTargetHandle.GetResourceHandle());
	framebuffer.renderTargetCount = 1u;
	if (depthStencilHandle.IsValid())
	{
		framebuffer.depthStencilView = ResolveDepthStencilView(depthStencilHandle.GetResourceHandle());
	}

	cmd.SetRenderTarget(
	    framebuffer.renderTargetViews[0],
	    framebuffer.HasDepthStencil() ? &framebuffer.depthStencilView : nullptr);
}

void FrameGraph::BindRenderTargets(
    RenderCommandContext& cmd,
    std::span<const FrameGraphTextureHandle> renderTargetHandles,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	assert(!renderTargetHandles.empty());
	assert(renderTargetHandles.size() <= 8u);

	FrameGraphFramebuffer framebuffer{};
	framebuffer.renderTargetCount = static_cast<std::uint32_t>(renderTargetHandles.size());
	for (std::size_t index = 0; index < renderTargetHandles.size(); ++index)
	{
		framebuffer.renderTargetViews[index] = ResolveRenderTargetView(renderTargetHandles[index].GetResourceHandle());
	}

	if (depthStencilHandle.IsValid())
	{
		framebuffer.depthStencilView = ResolveDepthStencilView(depthStencilHandle.GetResourceHandle());
	}

	cmd.SetRenderTargets(
	    framebuffer.renderTargetCount,
	    framebuffer.renderTargetViews.data(),
	    framebuffer.HasDepthStencil() ? &framebuffer.depthStencilView : nullptr);
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

void FrameGraph::CopyTexture(RenderCommandContext& cmd, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(cmd, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraph::CopyBuffer(RenderCommandContext& cmd, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(cmd, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraph::ClearRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	const std::array<float, 4> clearColor = GetClearColor(resourceHandle);
	cmd.ClearRenderTarget(ResolveRenderTargetView(resourceHandle), clearColor.data());
}

void FrameGraph::ClearDepthStencil(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	cmd.ClearDepthStencil(ResolveDepthStencilView(resourceHandle), GetClearDepth(resourceHandle));
}

NativeResourceHandle FrameGraph::ResolveResource(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveResource(handle.GetResourceHandle());
}

void FrameGraph::CopyResource(RenderCommandContext& cmd, FrameGraphResourceHandle destinationHandle, FrameGraphResourceHandle sourceHandle) const noexcept
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
