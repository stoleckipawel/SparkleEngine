#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"

#include <cstdint>
#include <vector>

class FrameGraphRecordingPlanCompiler final
{
  public:
	explicit FrameGraphRecordingPlanCompiler(FrameGraphPlan& plan) noexcept;

	void Compile();

  private:
	void InitializeResourceStates();
	void ApplyBarriers(const std::vector<FrameGraphBarrier>& barriers);
	void BuildGroups();
	void BuildGroup(
	    FrameGraphSubmissionBatch& batch,
	    FrameGraphPassIndex passIndex,
	    std::uint32_t submissionPosition);
	void BuildGroupStateContract(
	    RecordingGroup& group,
	    const FrameGraphPassNode& pass);
	void BuildGroupPrerequisites();
	void BuildChunks();
	void BuildBatchChunks(FrameGraphSubmissionBatch& batch);
	void CollapseBatchChunks(
	    FrameGraphSubmissionBatch& batch,
	    RecordingChunkIndex firstChunk);
	bool HasParallelRecordingRange(
	    const FrameGraphSubmissionBatch& batch) const noexcept;
	bool CanAppendToChunk(
	    const RecordingChunk& chunk,
	    const RecordingGroup& group) const noexcept;
	RecordingSerialIslandReason ResolveSerialIslandReason(
	    const FrameGraphPassNode& pass) const noexcept;
	std::uint32_t EstimateRecordingCost(
	    const FrameGraphPassNode& pass) const noexcept;
	bool WritesPresentationResource(
	    const FrameGraphPassNode& pass) const noexcept;
	static bool ContainsGroup(
	    const std::vector<RecordingGroupIndex>& groups,
	    RecordingGroupIndex group) noexcept;
	static bool ContainsResource(
	    const std::vector<RecordingResourceState>& states,
	    FrameGraphResourceHandle resource) noexcept;

	FrameGraphPlan& m_plan;
	std::vector<ResourceState> m_resourceStates;
	std::vector<RecordingGroupIndex> m_passToGroup;
};
