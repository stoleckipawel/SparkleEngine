#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "FrameGraph/Execution/FrameGraphRecordingExecutor.h"

#include <cstddef>
#include <array>
#include <span>

class FrameExecutionDiagnostics;
class FrameGraph;
class RenderCommandList;
class RhiCommandSubmissionService;
class TaskExecutor;

class FrameGraphSubmissionExecutor final
{
public:
	FrameGraphSubmissionExecutor(
	    const FrameGraph& frameGraph,
	    const FrameGraphPlan& plan,
	    RhiCommandSubmissionService& submissionService,
	    FrameExecutionDiagnostics& frameDiagnostics,
	    TaskExecutor& taskExecutor,
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
	RhiSubmissionToken RecordAndSubmitBatch(
	    const FrameGraphSubmissionBatch& batch,
	    const BatchWaitTokens& waits,
	    RhiCommandRecordingLease initializationLease);
	bool UsesNonGraphicsQueue() const noexcept;

	const FrameGraphPlan& m_plan;
	RhiCommandSubmissionService& m_submissionService;
	FrameGraphRecordingExecutor m_recordingExecutor;
	std::span<RhiSubmissionToken> m_batchTokens;
	RhiSubmissionToken m_initializationToken{};
	bool m_initialGraphicsListAvailable = true;
};
