#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "FrameGraph/Execution/FrameGraphRecordingChunkRecorder.h"
#include "RHI/Public/Commands/RhiCommandRecordingLease.h"

#include <array>
#include <cstdint>
#include <span>

class FrameExecutionDiagnostics;
class FrameGraph;
class RhiCommandSubmissionService;
class TaskExecutor;

struct RecordingChunkResult final
{
	RhiCommandRecordingLease Lease;
	bool UsesInitializationLease = false;
};

class FrameGraphRecordingExecutor final
{
public:
	FrameGraphRecordingExecutor(
	    const FrameGraph& frameGraph,
	    const FrameGraphPlan& plan,
	    RhiCommandSubmissionService& submissionService,
	    TaskExecutor& taskExecutor,
	    FrameExecutionDiagnostics& frameDiagnostics) noexcept;

	bool RecordBatch(const FrameGraphSubmissionBatch& batch, RhiCommandRecordingLease initializationLease = {});
	std::span<RhiCommandRecordingLease> Aggregate();

private:
	bool ShouldRecordBatchInParallel(const FrameGraphSubmissionBatch& batch) const noexcept;
	void RecordBatchSerial(const FrameGraphSubmissionBatch& batch, RhiCommandRecordingLease initializationLease);
	void AcquireBatchLeases(const FrameGraphSubmissionBatch& batch, RhiCommandRecordingLease initializationLease);
	bool RecordChunks();
	std::uint32_t FindParallelRangeEnd(std::uint32_t firstResult) const noexcept;
	void RecordSerialRange(std::uint32_t firstResult, std::uint32_t endResult);
	bool RecordParallelRange(std::uint32_t firstResult, std::uint32_t resultCount);
	void RecordChunk(std::uint32_t resultIndex);
	bool CanRecordParallel(const RecordingChunkResult& result, const RecordingChunk& chunk) const noexcept;
	bool ShouldExecuteParallelRange(std::uint32_t firstResult, std::uint32_t resultCount) const noexcept;
	std::uint32_t EstimateRangeCost(std::uint32_t firstResult, std::uint32_t resultCount) const noexcept;
	const RecordingChunk& GetChunk(std::uint32_t resultIndex) const noexcept;
	static std::uint64_t BuildTaskIdentity(SubmissionOrderKey submissionOrder) noexcept;

	const FrameGraphPlan& m_plan;
	RhiCommandSubmissionService& m_submissionService;
	TaskExecutor& m_taskExecutor;
	FrameGraphRecordingChunkRecorder m_chunkRecorder;
	std::array<RecordingChunkResult, RecordingPlan::MaximumChunksPerSubmissionBatch> m_results;
	std::array<RhiCommandRecordingLease, RecordingPlan::MaximumChunksPerSubmissionBatch> m_aggregatedLeases;
	RecordingChunkIndex m_firstChunk = InvalidRecordingChunkIndex;
	std::uint32_t m_resultCount = 0;
};
