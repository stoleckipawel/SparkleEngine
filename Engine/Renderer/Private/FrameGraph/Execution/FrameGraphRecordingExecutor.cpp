#include "PCH.h"

#include "FrameGraph/Execution/FrameGraphRecordingExecutor.h"

#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/Commands/RhiCommandSubmissionService.h"
#include "Tasks/Public/TaskExecutionContext.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskGraph.h"

#include <cassert>
#include <format>

FrameGraphRecordingExecutor::FrameGraphRecordingExecutor(
    const FrameGraph& frameGraph,
    const FrameGraphPlan& plan,
    RhiCommandSubmissionService& submissionService,
    TaskExecutor& taskExecutor,
    const FrameContext& frame,
    const PassRuntimeServices& passRuntimeServices,
    FrameExecutionDiagnostics& frameDiagnostics) noexcept :
	m_plan(plan),
	m_submissionService(submissionService),
	m_taskExecutor(taskExecutor),
	m_chunkRecorder(
	    frameGraph,
	    plan,
	    frame,
	    passRuntimeServices,
	    frameDiagnostics)
{
}

bool FrameGraphRecordingExecutor::RecordBatch(
    const FrameGraphSubmissionBatch& batch,
    RhiCommandRecordingLease initializationLease)
{
	AcquireBatchLeases(batch, std::move(initializationLease));
	return RecordChunks();
}

void FrameGraphRecordingExecutor::AcquireBatchLeases(
    const FrameGraphSubmissionBatch& batch,
    RhiCommandRecordingLease initializationLease)
{
	m_results.clear();
	m_results.resize(batch.recordingChunkCount);
	m_firstChunk = batch.recordingChunkOffset;

	for (std::uint32_t resultIndex = 0;
	     resultIndex < batch.recordingChunkCount;
	     ++resultIndex)
	{
		const RecordingChunk& chunk = GetChunk(resultIndex);
		RecordingChunkResult& result = m_results[resultIndex];
		result.SubmissionOrder = chunk.SubmissionOrder;

		if (resultIndex == 0 && initializationLease.IsValid())
		{
			result.Lease = std::move(initializationLease);
			result.UsesInitializationLease = true;
			continue;
		}

		const RhiCommandRecordingOwner owner =
		    chunk.ContextRequirement == RecordingContextRequirement::Coordinator
		        ? RhiCommandRecordingOwner{}
		        : RhiCommandRecordingOwner{
		              .WorkerIndex = resultIndex,
		              .TaskIdentity =
		                  BuildTaskIdentity(chunk.SubmissionOrder)};
		result.Lease =
		    m_submissionService.AcquireCommandRecordingLease(
		        chunk.Queue,
		        owner);
	}
}

bool FrameGraphRecordingExecutor::RecordChunks()
{
	std::uint32_t resultIndex = 0;
	while (resultIndex < m_results.size())
	{
		const RecordingChunk& chunk = GetChunk(resultIndex);
		if (!CanRecordParallel(m_results[resultIndex], chunk))
		{
			RecordChunk(resultIndex, true);
			++resultIndex;
			continue;
		}

		const std::uint32_t firstParallelResult = resultIndex;
		resultIndex = FindParallelRangeEnd(firstParallelResult);

		const std::uint32_t parallelResultCount = resultIndex - firstParallelResult;
		if (!ShouldExecuteParallelRange(firstParallelResult, parallelResultCount))
		{
			RecordSerialRange(firstParallelResult, resultIndex);
			continue;
		}

		if (!RecordParallelRange(firstParallelResult, parallelResultCount))
		{
			return false;
		}
	}

	return true;
}

std::uint32_t FrameGraphRecordingExecutor::FindParallelRangeEnd(
    std::uint32_t firstResult) const noexcept
{
	std::uint32_t endResult = firstResult;
	while (endResult < m_results.size() &&
	       CanRecordParallel(m_results[endResult], GetChunk(endResult)))
	{
		++endResult;
	}

	return endResult;
}

void FrameGraphRecordingExecutor::RecordSerialRange(
    std::uint32_t firstResult,
    std::uint32_t endResult)
{
	for (std::uint32_t resultIndex = firstResult;
	     resultIndex < endResult;
	     ++resultIndex)
	{
		RecordChunk(resultIndex, true);
	}
}

bool FrameGraphRecordingExecutor::RecordParallelRange(
    std::uint32_t firstResult,
    std::uint32_t resultCount)
{
	TaskGraphBuilder builder(
	    TaskGraphLimits{
	        .MaximumTasks = resultCount,
	        .MaximumEdges = 0});

	for (std::uint32_t resultOffset = 0;
	     resultOffset < resultCount;
	     ++resultOffset)
	{
		const std::uint32_t resultIndex = firstResult + resultOffset;
		const RecordingChunk& chunk = GetChunk(resultIndex);
		(void)builder.Add(
		    TaskDesc{
		        .Name =
		            TaskName(
		                std::format(
		                    "Renderer.FrameGraph.Record.{}.{}",
		                    chunk.SubmissionOrder.Batch,
		                    chunk.SubmissionOrder.Position)),
		        .Lane = TaskLane::FrameCritical},
		    [resultIndex](TaskExecutionContext& context)
		    {
			    FrameGraphRecordingExecutor* const executor =
			        context.TryGet<FrameGraphRecordingExecutor>();
			    if (executor == nullptr)
			    {
				    return TaskResult::Failure(
				        "Frame-graph recording task has no execution owner.");
			    }

			    executor->RecordChunk(resultIndex, false);
			    return TaskResult::Success();
		    });
	}

	const CompiledTaskGraph graph = builder.Compile();
	TaskExecutionContext context(*this);
	const TaskExecution execution =
	    m_taskExecutor.Submit(graph, context);
	return execution.GetStatus() == TaskExecutionStatus::Succeeded;
}

void FrameGraphRecordingExecutor::RecordChunk(
    std::uint32_t resultIndex,
    bool allowTiming)
{
	assert(resultIndex < m_results.size());
	RecordingChunkResult& result = m_results[resultIndex];
	assert(result.Lease.IsValid());

	m_chunkRecorder.Record(
	    GetChunk(resultIndex),
	    result.Lease.GetCommandList(),
	    allowTiming);
	result.Lease.Close();
}

bool FrameGraphRecordingExecutor::CanRecordParallel(
    const RecordingChunkResult& result,
    const RecordingChunk& chunk) const noexcept
{
	return CVarRendererParallelFrameGraphRecording.Get() &&
	       !result.UsesInitializationLease &&
	       chunk.ContextRequirement ==
	           RecordingContextRequirement::ExclusiveLease;
}

bool FrameGraphRecordingExecutor::ShouldExecuteParallelRange(
    std::uint32_t firstResult,
    std::uint32_t resultCount) const noexcept
{
	if (resultCount < 2 ||
	    m_taskExecutor.GetWorkerCount(TaskLane::FrameCritical) == 0)
	{
		return false;
	}

	return EstimateRangeCost(firstResult, resultCount) >=
	       RecordingPlan::MinimumParallelRecordingCost;
}

std::uint32_t FrameGraphRecordingExecutor::EstimateRangeCost(
    std::uint32_t firstResult,
    std::uint32_t resultCount) const noexcept
{
	std::uint32_t estimatedCost = 0;
	for (std::uint32_t resultOffset = 0;
	     resultOffset < resultCount;
	     ++resultOffset)
	{
		estimatedCost +=
		    GetChunk(firstResult + resultOffset)
		        .EstimatedRecordingCost;
	}

	return estimatedCost;
}

std::span<RhiCommandRecordingLease> FrameGraphRecordingExecutor::Aggregate()
{
	m_aggregatedLeases.clear();
	m_aggregatedLeases.reserve(m_results.size());

	for (RecordingChunkResult& result : m_results)
	{
		m_aggregatedLeases.push_back(
		    std::move(result.Lease));
	}

	return m_aggregatedLeases;
}

const RecordingChunk& FrameGraphRecordingExecutor::GetChunk(
    std::uint32_t resultIndex) const noexcept
{
	assert(m_firstChunk != InvalidRecordingChunkIndex);
	assert(resultIndex < m_results.size());
	return m_plan.recording.Chunks[m_firstChunk + resultIndex];
}

std::uint64_t FrameGraphRecordingExecutor::BuildTaskIdentity(
    SubmissionOrderKey submissionOrder) noexcept
{
	return static_cast<std::uint64_t>(submissionOrder.Batch) << 32u |
	       submissionOrder.Position;
}
