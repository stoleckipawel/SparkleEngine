#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Diagnostics/FrameGraphExecutionDiagnostics.h"


#include <format>
#include <string>

namespace
{
	static const auto g_frameGraphExecutionLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

	void FailMissingExecutionBinding(
	    std::string_view passName,
	    const FrameGraphResourceMetadata& resource,
	    bool hasResource,
	    RhiGpuVirtualAddress gpuAddress) noexcept
	{
		Diagnostics::Fail(
		    g_frameGraphExecutionLogger,
		    __FILE__,
		    __LINE__,
		    std::format(
		        "FrameGraph execution binding validation failed: pass='{}' resource='{}' ownership={} kind={} hasResource={} gpuAddress={} remediation='bind the external/persistent resource before execution or gate the pass so it does not declare the missing resource'",
		        passName,
		        resource.debugName.empty() ? "<unnamed>" : resource.debugName,
		        resource.ownership == FrameGraphResourceOwnership::Imported ? "Imported" : "ExternalPersistent",
		        resource.kind == FrameGraphResourceKind::AccelerationStructure ? "AccelerationStructure"
		        : resource.kind == FrameGraphResourceKind::Buffer              ? "Buffer"
		        : resource.kind == FrameGraphResourceKind::DepthStencil        ? "DepthStencil"
		                                                                      : "ColorRenderTarget",
		        hasResource,
		        gpuAddress));
	}
}

void FrameGraph::Execute(
    const FrameGraphPlan& plan,
    RenderCommandContext& cmd,
    const FrameContext& frame,
    const PassRuntimeServices& passRuntimeServices,
    FrameExecutionDiagnostics& frameDiagnostics) const
{
	FrameGraphExecutionDiagnostics graphDiagnostics(frameDiagnostics, cmd);
	if (graphDiagnostics.ShouldEmitDetailedMarkers())
	{
		cmd.EnableDrawDispatchDiagnostics();
	}

	EnsureTransientResourcesMaterialized(plan);
	ValidateExecutionBindings(plan);
	if (!plan.initialTransientAliasingBarriers.empty())
	{
		graphDiagnostics.InsertFrameBeginAliasingBarrierMarker();
	}
	EmitTransientAliasingBarriers(cmd, "FrameBegin", plan.initialTransientAliasingBarriers);

	for (const FrameGraphPassIndex passIndex : plan.executionOrder)
	{
		const FrameGraphPassNode& passRecord = plan.passes[passIndex];
		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (declaration.handle.IsValid())
			{
				cmd.GetRenderCommandList().TrackResource(m_resourceResolver.GetResolvedAccess(declaration.handle).resource);
			}
		}
		graphDiagnostics.InsertPassAliasingBarrierMarker(passRecord);
		EmitTransientAliasingBarriers(cmd, passRecord.passName, passRecord.transientAliasingBarriers);
		graphDiagnostics.InsertPassResourceBarrierMarker(passRecord);
		EmitCompiledBarriers(cmd, passRecord.passName, passRecord.compiledBarriers);
		PassExecutionDiagnostics passDiagnostics(
		    frameDiagnostics,
		    cmd,
		    passRecord.eventScopeLabel,
		    passRecord.passKind);
		auto passScope = graphDiagnostics.BeginPassScope(passDiagnostics);
		PassExecutionContext passContext{cmd, frame, passRuntimeServices, passDiagnostics, FrameGraphResourceCommands{*this}};
		m_passes[passIndex].executeCallback(passContext);
		if (passRecord.passKind == EFrameGraphPassFlags::ExternalProvider)
		{
			cmd.GetRenderCommandList().ResetBoundState();
		}
	}

	if (!plan.finalBarriers.empty())
	{
		graphDiagnostics.InsertFrameEndResourceBarrierMarker();
	}
	EmitCompiledBarriers(cmd, "FrameEnd", plan.finalBarriers);
	CommitTextureHistories();
}

void FrameGraph::ValidateExecutionBindings(const FrameGraphPlan& plan) const noexcept
{
	for (const FrameGraphPassIndex passIndex : plan.executionOrder)
	{
		const FrameGraphPassNode& passRecord = plan.passes[passIndex];
		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (!declaration.handle.IsValid() || !m_resourceRegistry.IsRegistered(declaration.handle))
			{
				continue;
			}

			const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(declaration.handle);
			if (!IsExternalFrameGraphResource(resource.ownership) || resource.kind == FrameGraphResourceKind::BackBuffer)
			{
				continue;
			}

			const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(resource.handle);
			const bool requiresGpuAddress = resource.kind == FrameGraphResourceKind::AccelerationStructure;
			const bool hasRequiredBinding = access.resource && (!requiresGpuAddress || access.accelerationStructureGpuAddress != 0);
			if (hasRequiredBinding)
			{
				continue;
			}

			FailMissingExecutionBinding(
			    passRecord.passName,
			    resource,
			    static_cast<bool>(access.resource),
			    access.accelerationStructureGpuAddress);
		}
	}
}
