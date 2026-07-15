#include "PCH.h"

#include "FrameGraph/Execution/FrameGraphSubmissionExecutor.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Diagnostics/FrameGraphExecutionDiagnostics.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <algorithm>
#include <array>
#include <format>
#include <span>

FrameGraphSubmissionExecutor::FrameGraphSubmissionExecutor(
	const FrameGraph& frameGraph,
	const FrameGraphPlan& plan,
	RhiCommandSubmissionService& submissionService,
	const FrameContext& frame,
	const PassRuntimeServices& passRuntimeServices,
	FrameExecutionDiagnostics& frameDiagnostics) noexcept :
	m_frameGraph(frameGraph),
	m_plan(plan),
	m_submissionService(submissionService),
	m_frame(frame),
	m_passRuntimeServices(passRuntimeServices),
	m_frameDiagnostics(frameDiagnostics),
	m_batchTokens(plan.submissionBatches.size())
{
}

RenderCommandList& FrameGraphSubmissionExecutor::Execute(RenderCommandList& initialGraphicsCommandList)
{
	m_initialGraphicsCommandList = &initialGraphicsCommandList;
	const bool usesNonGraphicsQueue = std::any_of(
	    m_plan.submissionBatches.begin(),
	    m_plan.submissionBatches.end(),
	    [](const FrameGraphSubmissionBatch& batch)
	    {
		    return batch.queue != ERhiQueueType::Graphics;
	    });
	if (usesNonGraphicsQueue)
	{
		// The prologue owns acquired-image and frame-initialization work. Async batches
		// wait only for that work instead of an unrelated first graphics batch.
		m_initializationToken = m_submissionService.SubmitCommandList(initialGraphicsCommandList);
		m_initialGraphicsListAvailable = false;
	}

	for (const FrameGraphSubmissionBatch& batch : m_plan.submissionBatches)
	{
		RhiSubmissionState waits;
		for (const FrameGraphSubmissionBatchIndex dependencyBatch : batch.waitForBatches)
		{
			if (dependencyBatch < m_batchTokens.size())
			{
				waits.MarkUsed(m_batchTokens[dependencyBatch]);
			}
		}
		if (batch.queue != ERhiQueueType::Graphics)
		{
			waits.MarkUsed(m_initializationToken);
		}

		std::array<RhiSubmissionToken, RhiQueueTypeCount> waitTokens{};
		const std::size_t waitTokenCount = waits.CopyTokens(waitTokens);
		RenderCommandList& commandList = AcquireCommandList(batch);
		RecordBatch(batch, commandList);
		m_batchTokens[batch.index] = m_submissionService.SubmitCommandList(
		    commandList,
		    std::span<const RhiSubmissionToken>(waitTokens.data(), waitTokenCount));
	}

	if (m_initialGraphicsListAvailable)
	{
		return m_submissionService.GetCurrentGraphicsCommandList();
	}
	return m_submissionService.BeginCommandList(ERhiQueueType::Graphics);
}

RenderCommandList& FrameGraphSubmissionExecutor::AcquireCommandList(const FrameGraphSubmissionBatch& batch)
{
	if (batch.queue == ERhiQueueType::Graphics && m_initialGraphicsListAvailable)
	{
		m_initialGraphicsListAvailable = false;
		return *m_initialGraphicsCommandList;
	}

	return m_submissionService.BeginCommandList(batch.queue);
}

void FrameGraphSubmissionExecutor::RecordBatch(
	const FrameGraphSubmissionBatch& batch,
	RenderCommandList& commandList) const
{
	RenderCommandContext commands(commandList);
	FrameGraphExecutionDiagnostics graphDiagnostics(m_frameDiagnostics, commands);
	if (graphDiagnostics.ShouldEmitDetailedMarkers())
	{
		commands.EnableDrawDispatchDiagnostics();
	}

	const std::string batchLabel = std::format(
	    "GPU Frame/{}/Batch {}",
	    RhiQueueTypeToString(batch.queue),
	    batch.index);
	auto batchScope = CVarRendererDiagnosticMarkerVerbosity.Get() != RendererDiagnosticMarkerVerbosity::Off
	                      ? m_frameDiagnostics.BeginGpuScope(
	                            commands,
	                            batchLabel,
	                            RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255})
	                      : ScopedGpuScope{};

	for (const FrameGraphPassIndex passIndex : batch.passes)
	{
		const FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
		for (const PassResourceDeclaration& declaration : passRecord.declarations)
		{
			if (declaration.handle.IsValid())
			{
				commandList.TrackResource(m_frameGraph.m_resourceResolver.GetResolvedAccess(declaration.handle).resource);
			}
		}

		graphDiagnostics.InsertPassAliasingBarrierMarker(passRecord);
		m_frameGraph.EmitTransientAliasingBarriers(commands, passRecord.passName, passRecord.transientAliasingBarriers);
		graphDiagnostics.InsertPassResourceBarrierMarker(passRecord);
		m_frameGraph.EmitCompiledBarriers(commands, passRecord.passName, passRecord.compiledBarriers);
		PassExecutionDiagnostics passDiagnostics(
		    m_frameDiagnostics,
		    commands,
		    passRecord.eventScopeLabel,
		    passRecord.kind);
		auto passScope = graphDiagnostics.BeginPassScope(passDiagnostics);
		PassExecutionContext passContext{
		    commands,
		    m_frame,
		    m_passRuntimeServices,
		    passDiagnostics,
		    FrameGraphResourceCommands{m_frameGraph}};
		m_frameGraph.m_passes[passIndex].executeCallback(passContext);
		if (passRecord.kind == EFrameGraphPassKind::ExternalProvider)
		{
			commandList.ResetBoundState();
		}
		m_frameGraph.EmitCompiledBarriers(commands, passRecord.passName, passRecord.compiledReleaseBarriers);
	}
}
