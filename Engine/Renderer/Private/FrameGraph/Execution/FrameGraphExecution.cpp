#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Diagnostics/FrameGraphExecutionDiagnostics.h"
#include "FrameGraph/Execution/FrameGraphSubmissionExecutor.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"

#include <format>
#include <string>

class FrameGraphExecutionOperations final
{
  public:
	inline static const auto g_frameGraphExecutionLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

	static void FailMissingExecutionBinding(
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

};

void FrameGraph::Execute(
    const FrameGraphPlan& plan,
    RhiCommandSubmissionService& submissionService,
    const FrameContext& frame,
    const PassRuntimeServices& passRuntimeServices,
    FrameExecutionDiagnostics& frameDiagnostics) const
{
	EnsureTransientResourcesMaterialized(plan);
	ValidateExecutionBindings(plan);
	RenderCommandList& initialGraphicsCommandList = submissionService.GetCurrentGraphicsCommandList();
	RenderCommandContext initialCommands(initialGraphicsCommandList);
	FrameGraphExecutionDiagnostics initialDiagnostics(frameDiagnostics, initialCommands);
	if (initialDiagnostics.ShouldEmitDetailedMarkers())
	{
		initialCommands.EnableDrawDispatchDiagnostics();
	}
	if (!plan.initialTransientAliasingBarriers.empty())
	{
		initialDiagnostics.InsertFrameBeginAliasingBarrierMarker();
	}
	EmitTransientAliasingBarriers(initialCommands, "FrameBegin", plan.initialTransientAliasingBarriers);
	EmitCompiledBarriers(initialCommands, "FrameBegin", plan.initialBarriers);

	FrameGraphSubmissionExecutor submissionExecutor(
	    *this,
	    plan,
	    submissionService,
	    frame,
	    passRuntimeServices,
	    frameDiagnostics);
	RenderCommandList& finalGraphicsCommandList = submissionExecutor.Execute(initialGraphicsCommandList);
	RenderCommandContext finalCommands(finalGraphicsCommandList);
	FrameGraphExecutionDiagnostics finalDiagnostics(frameDiagnostics, finalCommands);
	if (!plan.finalBarriers.empty())
	{
		finalDiagnostics.InsertFrameEndResourceBarrierMarker();
	}
	EmitCompiledBarriers(finalCommands, "FrameEnd", plan.finalBarriers);
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

			FrameGraphExecutionOperations::FailMissingExecutionBinding(
			    passRecord.passName,
			    resource,
			    static_cast<bool>(access.resource),
			    access.accelerationStructureGpuAddress);
		}
	}
}
