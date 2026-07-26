#include "PCH.h"

#include "FrameGraph/Execution/FrameGraphBatchRecorder.h"

#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Diagnostics/PassExecutionDiagnostics.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Diagnostics/FrameGraphExecutionDiagnostics.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <format>

FrameGraphBatchRecorder::FrameGraphBatchRecorder(
    const FrameGraph& frameGraph,
    const FrameGraphPlan& plan,
    const FrameContext& frame,
    const PassRuntimeServices& passRuntimeServices,
    FrameExecutionDiagnostics& frameDiagnostics) noexcept :
	m_frameGraph(frameGraph),
	m_plan(plan),
	m_frame(frame),
	m_passRuntimeServices(passRuntimeServices),
	m_frameDiagnostics(frameDiagnostics)
{
}

void FrameGraphBatchRecorder::Record(
    const FrameGraphSubmissionBatch& batch,
    RenderCommandList& commandList) const
{
	RenderCommandContext commands(commandList);
	FrameGraphExecutionDiagnostics graphDiagnostics(m_frameDiagnostics, commands);
	if (graphDiagnostics.ShouldEmitDetailedMarkers())
	{
		commands.EnableDrawDispatchDiagnostics();
	}

	auto batchScope = BeginBatchScope(batch, commands);
	for (const FrameGraphPassIndex passIndex : batch.passes)
	{
		RecordPass(passIndex, commandList, commands, graphDiagnostics);
	}
}

ScopedGpuScope FrameGraphBatchRecorder::BeginBatchScope(
    const FrameGraphSubmissionBatch& batch,
    RenderCommandContext& commands) const
{
	if (CVarRendererDiagnosticMarkerVerbosity.Get() == RendererDiagnosticMarkerVerbosity::Off)
	{
		return {};
	}

	const std::string batchLabel = std::format(
	    "GPU Frame/{}/Batch {}",
	    RhiQueueTypeToString(batch.queue),
	    batch.index);
	return m_frameDiagnostics.BeginGpuScope(
	    commands,
	    batchLabel,
	    RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255});
}

void FrameGraphBatchRecorder::RecordPass(
    FrameGraphPassIndex passIndex,
    RenderCommandList& commandList,
    RenderCommandContext& commands,
    FrameGraphExecutionDiagnostics& graphDiagnostics) const
{
	const FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
	TrackPassResources(passRecord, commandList);

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
	const FrameGraph::RegisteredPass& registeredPass = m_frameGraph.m_passes[passIndex];
	if (registeredPass.active)
	{
		registeredPass.executeCallback(passContext);
	}

	if (passRecord.kind == EFrameGraphPassKind::ExternalProvider)
	{
		commandList.ResetBoundState();
	}

	m_frameGraph.EmitCompiledBarriers(commands, passRecord.passName, passRecord.compiledReleaseBarriers);
}

void FrameGraphBatchRecorder::TrackPassResources(
    const FrameGraphPassNode& pass,
    RenderCommandList& commandList) const
{
	for (const PassResourceDeclaration& declaration : pass.declarations)
	{
		if (declaration.handle.IsValid())
		{
			commandList.TrackResource(m_frameGraph.m_resourceResolver.GetResolvedAccess(declaration.handle).resource);
		}
	}
}
