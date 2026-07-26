#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "FrameGraph/Execution/FrameGraphBatchRecorder.h"

#include <array>
#include <span>

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
	    FrameExecutionDiagnostics& frameDiagnostics,
	    std::span<RhiSubmissionToken> batchTokens) noexcept;

	RenderCommandList& Execute(RenderCommandList& initialGraphicsCommandList);

  private:
	struct BatchWaitTokens final
	{
		std::array<RhiSubmissionToken, RhiQueueTypeCount> Values = {};
		std::size_t Count = 0;
	};

	void SubmitInitializationIfRequired();
	BatchWaitTokens ResolveBatchWaits(const FrameGraphSubmissionBatch& batch) const noexcept;
	void ExecuteBatch(const FrameGraphSubmissionBatch& batch);
	RhiSubmissionToken RecordAndSubmitCurrentGraphicsBatch(
	    const FrameGraphSubmissionBatch& batch,
	    const BatchWaitTokens& waits);
	RhiSubmissionToken RecordAndSubmitLeasedBatch(
	    const FrameGraphSubmissionBatch& batch,
	    const BatchWaitTokens& waits);
	bool UsesNonGraphicsQueue() const noexcept;

	const FrameGraphPlan& m_plan;
	RhiCommandSubmissionService& m_submissionService;
	FrameGraphBatchRecorder m_batchRecorder;
	std::span<RhiSubmissionToken> m_batchTokens;
	RhiSubmissionToken m_initializationToken{};
	RenderCommandList* m_initialGraphicsCommandList = nullptr;
	bool m_initialGraphicsListAvailable = true;
};
