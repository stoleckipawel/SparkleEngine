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
	const PassRuntimeServices& passRuntimeServices,
	FrameExecutionDiagnostics& frameDiagnostics,
	std::span<RhiSubmissionToken> batchTokens) noexcept :
	m_plan(plan),
	m_submissionService(submissionService),
	m_batchRecorder(frameGraph, plan, frame, passRuntimeServices, frameDiagnostics),
	m_batchTokens(batchTokens)
{
}

RenderCommandList& FrameGraphSubmissionExecutor::Execute(RenderCommandList& initialGraphicsCommandList)
{
	m_initialGraphicsCommandList = &initialGraphicsCommandList;

	SubmitInitializationIfRequired();

	for (const FrameGraphSubmissionBatch& batch : m_plan.submissionBatches)
	{
		ExecuteBatch(batch);
	}

	if (m_initialGraphicsListAvailable)
	{
		return m_submissionService.GetCurrentGraphicsCommandList();
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
	if (usesCurrentGraphics)
	{
		m_batchTokens[batch.index] = RecordAndSubmitCurrentGraphicsBatch(batch, waits);
	}
	else
	{
		m_batchTokens[batch.index] = RecordAndSubmitLeasedBatch(batch, waits);
	}
}

RhiSubmissionToken FrameGraphSubmissionExecutor::RecordAndSubmitCurrentGraphicsBatch(
    const FrameGraphSubmissionBatch& batch,
    const BatchWaitTokens& waits)
{
	m_initialGraphicsListAvailable = false;
	m_batchRecorder.Record(batch, *m_initialGraphicsCommandList);
	return m_submissionService.SubmitCurrentGraphicsCommandList(
	    std::span<const RhiSubmissionToken>(waits.Values.data(), waits.Count));
}

RhiSubmissionToken FrameGraphSubmissionExecutor::RecordAndSubmitLeasedBatch(
    const FrameGraphSubmissionBatch& batch,
    const BatchWaitTokens& waits)
{
	RhiCommandRecordingLease lease = m_submissionService.AcquireCommandRecordingLease(batch.queue);
	m_batchRecorder.Record(batch, lease.GetCommandList());
	return m_submissionService.SubmitCommandRecordingLease(
	    std::move(lease),
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
