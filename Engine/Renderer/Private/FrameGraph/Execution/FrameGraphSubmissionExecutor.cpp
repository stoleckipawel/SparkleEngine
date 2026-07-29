#include "PCH.h"

#include "FrameGraph/Execution/FrameGraphSubmissionExecutor.h"

#include "RHI/Public/Commands/RhiCommandSubmissionService.h"

#include <algorithm>
#include <span>

FrameGraphSubmissionExecutor::FrameGraphSubmissionExecutor(
	const FrameGraph& frameGraph,
	const FrameGraphPlan& plan,
	RhiCommandSubmissionService& submissionService,
	const FrameContext& frame,
	const PassRuntimeContext& passRuntimeContext,
	FrameExecutionDiagnostics& frameDiagnostics,
	TaskExecutor& taskExecutor,
	std::span<RhiSubmissionToken> batchTokens) noexcept :
	m_plan(plan),
	m_submissionService(submissionService),
	m_recordingExecutor(
	    frameGraph,
	    plan,
	    submissionService,
	    taskExecutor,
	    frame,
	    passRuntimeContext,
	    frameDiagnostics),
	m_batchTokens(batchTokens)
{
}

RenderCommandList& FrameGraphSubmissionExecutor::Execute(RenderCommandList& initialGraphicsCommandList)
{
	SubmitInitializationIfRequired();

	for (const FrameGraphSubmissionBatch& batch : m_plan.submissionBatches)
	{
		ExecuteBatch(batch);
	}

	if (m_initialGraphicsListAvailable)
	{
		return initialGraphicsCommandList;
	}

	return m_submissionService.BeginCurrentGraphicsCommandList();
}

void FrameGraphSubmissionExecutor::SubmitInitializationIfRequired()
{
	if (!UsesNonGraphicsQueue())
	{
		return;
	}

	m_initializationToken = m_submissionService.SubmitCurrentGraphicsCommandList();
	m_initialGraphicsListAvailable = false;
}

FrameGraphSubmissionExecutor::BatchWaitTokens FrameGraphSubmissionExecutor::ResolveBatchWaits(
    const FrameGraphSubmissionBatch& batch) const noexcept
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

	BatchWaitTokens result;
	result.Count = waits.CopyTokens(result.Values);
	return result;
}

void FrameGraphSubmissionExecutor::ExecuteBatch(const FrameGraphSubmissionBatch& batch)
{
	const BatchWaitTokens waits = ResolveBatchWaits(batch);
	const bool usesCurrentGraphics =
	    batch.queue == ERhiQueueType::Graphics && m_initialGraphicsListAvailable;
	RhiCommandRecordingLease initializationLease;
	if (usesCurrentGraphics)
	{
		initializationLease =
		    m_submissionService
		        .TakeCurrentGraphicsCommandRecordingLease();
		m_initialGraphicsListAvailable = false;
	}

	m_batchTokens[batch.index] =
	    RecordAndSubmitBatch(
	        batch,
	        waits,
	        std::move(initializationLease));
}

RhiSubmissionToken FrameGraphSubmissionExecutor::RecordAndSubmitBatch(
    const FrameGraphSubmissionBatch& batch,
    const BatchWaitTokens& waits,
    RhiCommandRecordingLease initializationLease)
{
	if (!m_recordingExecutor.RecordBatch(
	        batch,
	        std::move(initializationLease)))
	{
		return {};
	}

	return m_submissionService.SubmitCommandRecordingBatch(
	    m_recordingExecutor.Aggregate(),
	    std::span<const RhiSubmissionToken>(waits.Values.data(), waits.Count));
}

bool FrameGraphSubmissionExecutor::UsesNonGraphicsQueue() const noexcept
{
	return std::any_of(
	    m_plan.submissionBatches.begin(),
	    m_plan.submissionBatches.end(),
	    [](const FrameGraphSubmissionBatch& batch)
	    {
		    return batch.queue != ERhiQueueType::Graphics;
	    });
}
