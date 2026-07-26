#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphRecordingPlanCompiler.h"

#include <algorithm>
#include <cassert>

FrameGraphRecordingPlanCompiler::FrameGraphRecordingPlanCompiler(
    FrameGraphPlan& plan) noexcept :
	m_plan(plan)
{
}

void FrameGraphRecordingPlanCompiler::Compile()
{
	m_plan.recording.Clear();

	InitializeResourceStates();
	BuildGroups();
	BuildGroupPrerequisites();
	BuildChunks();
}

void FrameGraphRecordingPlanCompiler::InitializeResourceStates()
{
	m_resourceStates.clear();
	m_resourceStates.reserve(m_plan.resources.size());
	for (const FrameGraphResourceNode& resource : m_plan.resources)
	{
		m_resourceStates.push_back(resource.planningStartState);
	}

	ApplyBarriers(m_plan.initialBarriers);
}

void FrameGraphRecordingPlanCompiler::ApplyBarriers(
    const std::vector<FrameGraphBarrier>& barriers)
{
	for (const FrameGraphBarrier& barrier : barriers)
	{
		if (barrier.type != FrameGraphBarrier::Type::Transition ||
		    !barrier.handle.IsValid())
		{
			continue;
		}

		assert(barrier.handle.index < m_resourceStates.size());
		m_resourceStates[barrier.handle.index] = barrier.after;
	}
}

void FrameGraphRecordingPlanCompiler::BuildGroups()
{
	m_passToGroup.assign(
	    m_plan.passes.size(),
	    InvalidRecordingGroupIndex);

	for (FrameGraphSubmissionBatch& batch : m_plan.submissionBatches)
	{
		for (std::uint32_t position = 0; position < batch.passes.size(); ++position)
		{
			BuildGroup(batch, batch.passes[position], position);
		}
	}
}

void FrameGraphRecordingPlanCompiler::BuildGroup(
    FrameGraphSubmissionBatch& batch,
    FrameGraphPassIndex passIndex,
    std::uint32_t submissionPosition)
{
	assert(passIndex < m_plan.passes.size());
	const FrameGraphPassNode& pass = m_plan.passes[passIndex];
	const RecordingSerialIslandReason serialReason =
	    ResolveSerialIslandReason(pass);

	RecordingGroup group{
	    .Index = static_cast<RecordingGroupIndex>(m_plan.recording.Groups.size()),
	    .PassOffset = static_cast<std::uint32_t>(m_plan.recording.Passes.size()),
	    .PassCount = 1,
	    .Queue = batch.queue,
	    .ContextRequirement =
	        serialReason == RecordingSerialIslandReason::None
	            ? RecordingContextRequirement::ExclusiveLease
	            : RecordingContextRequirement::Coordinator,
	    .EstimatedRecordingCost = EstimateRecordingCost(pass),
	    .SubmissionOrder =
	        SubmissionOrderKey{
	            .Batch = batch.index,
	            .Position = submissionPosition},
	    .SerialIslandReason = serialReason};

	BuildGroupStateContract(group, pass);

	m_plan.recording.Passes.push_back(passIndex);
	m_passToGroup[passIndex] = group.Index;
	m_plan.recording.Groups.push_back(std::move(group));
}

void FrameGraphRecordingPlanCompiler::BuildGroupStateContract(
    RecordingGroup& group,
    const FrameGraphPassNode& pass)
{
	for (const PassResourceDeclaration& declaration : pass.declarations)
	{
		if (!declaration.handle.IsValid() ||
		    ContainsResource(group.InitialResourceStates, declaration.handle))
		{
			continue;
		}

		assert(declaration.handle.index < m_resourceStates.size());
		group.InitialResourceStates.push_back(
		    RecordingResourceState{
		        .Resource = declaration.handle,
		        .State = m_resourceStates[declaration.handle.index]});
	}

	ApplyBarriers(pass.compiledBarriers);
	ApplyBarriers(pass.compiledReleaseBarriers);

	for (const RecordingResourceState& initialState : group.InitialResourceStates)
	{
		group.FinalResourceStates.push_back(
		    RecordingResourceState{
		        .Resource = initialState.Resource,
		        .State = m_resourceStates[initialState.Resource.index]});
	}
}

void FrameGraphRecordingPlanCompiler::BuildGroupPrerequisites()
{
	for (RecordingGroup& group : m_plan.recording.Groups)
	{
		assert(group.PassCount == 1);
		const FrameGraphPassIndex passIndex =
		    m_plan.recording.Passes[group.PassOffset];
		const FrameGraphPassNode& pass = m_plan.passes[passIndex];

		for (const FrameGraphPassIndex dependency : pass.synchronizationDependencies)
		{
			assert(dependency < m_passToGroup.size());
			const RecordingGroupIndex prerequisite = m_passToGroup[dependency];
			if (prerequisite != InvalidRecordingGroupIndex &&
			    prerequisite != group.Index &&
			    !ContainsGroup(group.Prerequisites, prerequisite))
			{
				group.Prerequisites.push_back(prerequisite);
			}
		}
	}
}

void FrameGraphRecordingPlanCompiler::BuildChunks()
{
	for (FrameGraphSubmissionBatch& batch : m_plan.submissionBatches)
	{
		BuildBatchChunks(batch);
	}
}

void FrameGraphRecordingPlanCompiler::BuildBatchChunks(
    FrameGraphSubmissionBatch& batch)
{
	const RecordingChunkIndex firstChunk =
	    static_cast<RecordingChunkIndex>(m_plan.recording.Chunks.size());
	batch.recordingChunkOffset = firstChunk;

	for (const FrameGraphPassIndex passIndex : batch.passes)
	{
		assert(passIndex < m_passToGroup.size());
		const RecordingGroupIndex groupIndex = m_passToGroup[passIndex];
		assert(groupIndex != InvalidRecordingGroupIndex);
		const RecordingGroup& group = m_plan.recording.Groups[groupIndex];

		RecordingChunk* chunk =
		    m_plan.recording.Chunks.size() > firstChunk
		        ? &m_plan.recording.Chunks.back()
		        : nullptr;
		if (chunk != nullptr && CanAppendToChunk(*chunk, group))
		{
			++chunk->GroupCount;
			chunk->EstimatedRecordingCost += group.EstimatedRecordingCost;
			continue;
		}

		m_plan.recording.Chunks.push_back(
		    RecordingChunk{
		        .Index = static_cast<RecordingChunkIndex>(m_plan.recording.Chunks.size()),
		        .FirstGroup = group.Index,
		        .GroupCount = 1,
		        .Queue = group.Queue,
		        .ContextRequirement = group.ContextRequirement,
		        .EstimatedRecordingCost = group.EstimatedRecordingCost,
		        .SubmissionOrder = group.SubmissionOrder});
	}

	batch.recordingChunkCount =
	    static_cast<std::uint32_t>(m_plan.recording.Chunks.size() - firstChunk);
	if (batch.recordingChunkCount >
	        RecordingPlan::MaximumChunksPerSubmissionBatch ||
	    !HasParallelRecordingRange(batch))
	{
		CollapseBatchChunks(batch, firstChunk);
	}
}

void FrameGraphRecordingPlanCompiler::CollapseBatchChunks(
    FrameGraphSubmissionBatch& batch,
    RecordingChunkIndex firstChunk)
{
	assert(!batch.passes.empty());
	const RecordingGroupIndex firstGroup =
	    m_passToGroup[batch.passes.front()];
	std::uint32_t estimatedCost = 0;
	for (const FrameGraphPassIndex passIndex : batch.passes)
	{
		estimatedCost += m_plan.recording.Groups[m_passToGroup[passIndex]].EstimatedRecordingCost;
	}

	m_plan.recording.Chunks.resize(firstChunk);
	m_plan.recording.Chunks.push_back(
	    RecordingChunk{
	        .Index = firstChunk,
	        .FirstGroup = firstGroup,
	        .GroupCount = static_cast<std::uint32_t>(batch.passes.size()),
	        .Queue = batch.queue,
	        .ContextRequirement = RecordingContextRequirement::Coordinator,
	        .EstimatedRecordingCost = estimatedCost,
	        .SubmissionOrder =
	            SubmissionOrderKey{
	                .Batch = batch.index,
	                .Position = 0}});

	batch.recordingChunkCount = 1;
}

bool FrameGraphRecordingPlanCompiler::HasParallelRecordingRange(
    const FrameGraphSubmissionBatch& batch) const noexcept
{
	std::uint32_t rangeChunkCount = 0;
	std::uint32_t rangeCost = 0;
	const RecordingChunkIndex endChunk =
	    batch.recordingChunkOffset +
	    batch.recordingChunkCount;

	for (RecordingChunkIndex chunkIndex =
	         batch.recordingChunkOffset;
	     chunkIndex < endChunk;
	     ++chunkIndex)
	{
		const RecordingChunk& chunk =
		    m_plan.recording.Chunks[chunkIndex];
		if (chunk.ContextRequirement !=
		    RecordingContextRequirement::ExclusiveLease)
		{
			rangeChunkCount = 0;
			rangeCost = 0;
			continue;
		}

		++rangeChunkCount;
		rangeCost += chunk.EstimatedRecordingCost;
		if (rangeChunkCount >= 2 &&
		    rangeCost >=
		        RecordingPlan::MinimumParallelRecordingCost)
		{
			return true;
		}
	}

	return false;
}

bool FrameGraphRecordingPlanCompiler::CanAppendToChunk(
    const RecordingChunk& chunk,
    const RecordingGroup& group) const noexcept
{
	if (chunk.Queue != group.Queue ||
	    chunk.ContextRequirement != group.ContextRequirement)
	{
		return false;
	}

	return chunk.ContextRequirement == RecordingContextRequirement::Coordinator ||
	       chunk.EstimatedRecordingCost <
	           RecordingPlan::TargetParallelChunkCost;
}

RecordingSerialIslandReason
FrameGraphRecordingPlanCompiler::ResolveSerialIslandReason(
    const FrameGraphPassNode& pass) const noexcept
{
	if (pass.kind == EFrameGraphPassKind::ExternalProvider)
	{
		return RecordingSerialIslandReason::ExternalProvider;
	}

	if (pass.executionModel != FrameGraphPassExecutionModel::TypedShader)
	{
		return RecordingSerialIslandReason::Callback;
	}

	return WritesPresentationResource(pass)
	           ? RecordingSerialIslandReason::Presentation
	           : RecordingSerialIslandReason::None;
}

std::uint32_t FrameGraphRecordingPlanCompiler::EstimateRecordingCost(
    const FrameGraphPassNode& pass) const noexcept
{
	return 1u +
	       static_cast<std::uint32_t>(pass.declarations.size()) +
	       static_cast<std::uint32_t>(pass.transientAliasingBarriers.size()) +
	       static_cast<std::uint32_t>(pass.compiledBarriers.size()) +
	       static_cast<std::uint32_t>(pass.compiledReleaseBarriers.size());
}

bool FrameGraphRecordingPlanCompiler::WritesPresentationResource(
    const FrameGraphPassNode& pass) const noexcept
{
	for (const PassResourceDeclaration& declaration : pass.declarations)
	{
		if (declaration.usage == ResourceUsage::Present)
		{
			return true;
		}

		if (!declaration.handle.IsValid() ||
		    !WritesToUsage(declaration.usage))
		{
			continue;
		}

		assert(declaration.handle.index < m_plan.resources.size());
		if (m_plan.resources[declaration.handle.index].kind ==
		    FrameGraphResourceKind::BackBuffer)
		{
			return true;
		}
	}

	return false;
}

bool FrameGraphRecordingPlanCompiler::ContainsGroup(
    const std::vector<RecordingGroupIndex>& groups,
    RecordingGroupIndex group) noexcept
{
	return std::find(groups.begin(), groups.end(), group) != groups.end();
}

bool FrameGraphRecordingPlanCompiler::ContainsResource(
    const std::vector<RecordingResourceState>& states,
    FrameGraphResourceHandle resource) noexcept
{
	return std::find_if(
	           states.begin(),
	           states.end(),
	           [resource](const RecordingResourceState& state)
	           {
		           return state.Resource == resource;
	           }) != states.end();
}
