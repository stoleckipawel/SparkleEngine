#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"

#include <vector>

class FrameExecutionDiagnostics;
class FrameGraph;
class RenderCommandList;
class RhiCommandSubmissionService;
struct FrameContext;
struct PassRuntimeServices;

class FrameGraphSubmissionExecutor final
{
  public:
	FrameGraphSubmissionExecutor(
	    const FrameGraph& frameGraph,
	    const FrameGraphPlan& plan,
	    RhiCommandSubmissionService& submissionService,
	    const FrameContext& frame,
	    const PassRuntimeServices& passRuntimeServices,
	    FrameExecutionDiagnostics& frameDiagnostics) noexcept;

	RenderCommandList& Execute(RenderCommandList& initialGraphicsCommandList);

  private:
	RenderCommandList& AcquireCommandList(const FrameGraphSubmissionBatch& batch);
	void RecordBatch(const FrameGraphSubmissionBatch& batch, RenderCommandList& commandList) const;

	const FrameGraph& m_frameGraph;
	const FrameGraphPlan& m_plan;
	RhiCommandSubmissionService& m_submissionService;
	const FrameContext& m_frame;
	const PassRuntimeServices& m_passRuntimeServices;
	FrameExecutionDiagnostics& m_frameDiagnostics;
	std::vector<RhiSubmissionToken> m_batchTokens;
	RhiSubmissionToken m_initializationToken{};
	RenderCommandList* m_initialGraphicsCommandList = nullptr;
	bool m_initialGraphicsListAvailable = true;
};
