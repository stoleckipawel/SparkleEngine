#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "FrameGraph/Diagnostics/FrameGraphExecutionDiagnostics.h"
#include "FrameGraph/Execution/FrameGraphSubmissionExecutor.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include <algorithm>

void FrameGraph::Execute(
    const FrameGraphPlan& plan,
    RhiCommandSubmissionService& submissionService,
    FrameExecutionDiagnostics& frameDiagnostics,
    TaskExecutor& taskExecutor) const
{
	EnsureTransientResourcesMaterialized(plan);
	submissionService.PrepareCommandRecording();

	std::fill(m_submissionBatchTokens.begin(), m_submissionBatchTokens.end(), RhiSubmissionToken{});

	RenderCommandList& initialGraphicsCommandList = submissionService.GetCurrentGraphicsCommandList();
	RecordFrameBeginBarriers(plan, initialGraphicsCommandList, frameDiagnostics);

	FrameGraphSubmissionExecutor
	    submissionExecutor(*this, plan, submissionService, frameDiagnostics, taskExecutor, m_submissionBatchTokens);
	RenderCommandList& finalGraphicsCommandList = submissionExecutor.Execute(initialGraphicsCommandList);
	RecordFrameEndBarriers(plan, finalGraphicsCommandList, frameDiagnostics);

	CommitTextureHistories();
}

void FrameGraph::RecordFrameBeginBarriers(
    const FrameGraphPlan& plan,
    RenderCommandList& commandList,
    FrameExecutionDiagnostics& frameDiagnostics) const
{
	RenderCommandContext commands(commandList);
	FrameGraphExecutionDiagnostics diagnostics(frameDiagnostics, commands);
	if (diagnostics.ShouldEmitDetailedMarkers())
	{
		commands.EnableDrawDispatchDiagnostics();
	}

	if (!plan.initialTransientAliasingBarriers.empty())
	{
		diagnostics.InsertFrameBeginAliasingBarrierMarker();
	}

	EmitTransientAliasingBarriers(commands, "FrameBegin", plan.initialTransientAliasingBarriers);
	EmitCompiledBarriers(commands, "FrameBegin", plan.initialBarriers);
}

void FrameGraph::RecordFrameEndBarriers(
    const FrameGraphPlan& plan,
    RenderCommandList& commandList,
    FrameExecutionDiagnostics& frameDiagnostics) const
{
	RenderCommandContext commands(commandList);
	FrameGraphExecutionDiagnostics diagnostics(frameDiagnostics, commands);
	if (!plan.finalBarriers.empty())
	{
		diagnostics.InsertFrameEndResourceBarrierMarker();
	}

	EmitCompiledBarriers(commands, "FrameEnd", plan.finalBarriers);
}
