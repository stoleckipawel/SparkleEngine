#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphCompiler.h"

#include <algorithm>
#include <cassert>

void FrameGraphCompiler::AssignPassQueues() noexcept
{
	for (FrameGraphPassNode& passRecord : m_plan.passes)
	{
		passRecord.queue = ERhiQueueType::Graphics;
		if (!passRecord.alive)
		{
			continue;
		}

		if (passRecord.queuePreference == EFrameGraphQueuePreference::AsyncCompute
		    && m_queueCapabilities.SupportsIndependent(ERhiQueueType::Compute))
		{
			assert(passRecord.kind == EFrameGraphPassKind::Compute);
			passRecord.queue = ERhiQueueType::Compute;
		}
		else if (passRecord.queuePreference == EFrameGraphQueuePreference::Copy
		    && m_queueCapabilities.SupportsIndependent(ERhiQueueType::Copy))
		{
			assert(passRecord.kind == EFrameGraphPassKind::Transfer);
			passRecord.queue = ERhiQueueType::Copy;
		}
	}
}

void FrameGraphCompiler::AddSynchronizationDependency(FrameGraphPassNode& passRecord, FrameGraphPassIndex dependency) noexcept
{
	if (dependency == INVALID_FRAME_GRAPH_PASS_INDEX || dependency == passRecord.index)
	{
		return;
	}
	if (std::find(passRecord.synchronizationDependencies.begin(), passRecord.synchronizationDependencies.end(), dependency)
	    == passRecord.synchronizationDependencies.end())
	{
		passRecord.synchronizationDependencies.push_back(dependency);
	}
}

void FrameGraphCompiler::BuildSubmissionBatches() noexcept
{
	m_plan.submissionBatches.clear();
	std::vector<FrameGraphSubmissionBatchIndex> passToBatch(m_plan.passes.size(), INVALID_FRAME_GRAPH_SUBMISSION_BATCH_INDEX);

	for (const FrameGraphPassIndex passIndex : m_plan.executionOrder)
	{
		FrameGraphPassNode& passRecord = m_plan.passes[passIndex];
		for (const FrameGraphAliasingBarrier& aliasingBarrier : passRecord.transientAliasingBarriers)
		{
			AddSynchronizationDependency(passRecord, aliasingBarrier.executeBeforePass);
		}

		std::vector<FrameGraphSubmissionBatchIndex> crossQueueDependencies;
		for (const FrameGraphPassIndex dependencyPass : passRecord.synchronizationDependencies)
		{
			assert(dependencyPass < passToBatch.size());
			const FrameGraphSubmissionBatchIndex dependencyBatch = passToBatch[dependencyPass];
			assert(dependencyBatch != INVALID_FRAME_GRAPH_SUBMISSION_BATCH_INDEX);
			if (m_plan.submissionBatches[dependencyBatch].queue == passRecord.queue)
			{
				continue;
			}
			if (std::find(crossQueueDependencies.begin(), crossQueueDependencies.end(), dependencyBatch) == crossQueueDependencies.end())
			{
				crossQueueDependencies.push_back(dependencyBatch);
			}
		}

		bool startNewBatch = m_plan.submissionBatches.empty() || m_plan.submissionBatches.back().queue != passRecord.queue;
		if (!startNewBatch)
		{
			const FrameGraphSubmissionBatch& currentBatch = m_plan.submissionBatches.back();
			for (const FrameGraphSubmissionBatchIndex dependencyBatch : crossQueueDependencies)
			{
				if (std::find(currentBatch.waitForBatches.begin(), currentBatch.waitForBatches.end(), dependencyBatch)
				    == currentBatch.waitForBatches.end())
				{
					startNewBatch = true;
					break;
				}
			}
		}

		if (startNewBatch)
		{
			const auto batchIndex = static_cast<FrameGraphSubmissionBatchIndex>(m_plan.submissionBatches.size());
			m_plan.submissionBatches.push_back(
			    FrameGraphSubmissionBatch{
			        .index = batchIndex,
			        .queue = passRecord.queue,
			        .passes = {},
			        .waitForBatches = std::move(crossQueueDependencies)});
		}

		FrameGraphSubmissionBatch& batch = m_plan.submissionBatches.back();
		batch.passes.push_back(passIndex);
		passToBatch[passIndex] = batch.index;
	}
}
