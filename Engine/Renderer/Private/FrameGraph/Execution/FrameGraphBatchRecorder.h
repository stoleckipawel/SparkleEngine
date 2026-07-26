#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"

class FrameExecutionDiagnostics;
class FrameGraph;
class FrameGraphExecutionDiagnostics;
class RenderCommandContext;
class RenderCommandList;
class ScopedGpuScope;
struct FrameContext;
struct PassRuntimeServices;

class FrameGraphBatchRecorder final
{
  public:
	FrameGraphBatchRecorder(
	    const FrameGraph& frameGraph,
	    const FrameGraphPlan& plan,
	    const FrameContext& frame,
	    const PassRuntimeServices& passRuntimeServices,
	    FrameExecutionDiagnostics& frameDiagnostics) noexcept;

	void Record(const FrameGraphSubmissionBatch& batch, RenderCommandList& commandList) const;

  private:
	ScopedGpuScope BeginBatchScope(
	    const FrameGraphSubmissionBatch& batch,
	    RenderCommandContext& commands) const;
	void RecordPass(
	    FrameGraphPassIndex passIndex,
	    RenderCommandList& commandList,
	    RenderCommandContext& commands,
	    FrameGraphExecutionDiagnostics& graphDiagnostics) const;
	void TrackPassResources(
	    const FrameGraphPassNode& pass,
	    RenderCommandList& commandList) const;

	const FrameGraph& m_frameGraph;
	const FrameGraphPlan& m_plan;
	const FrameContext& m_frame;
	const PassRuntimeServices& m_passRuntimeServices;
	FrameExecutionDiagnostics& m_frameDiagnostics;
};
