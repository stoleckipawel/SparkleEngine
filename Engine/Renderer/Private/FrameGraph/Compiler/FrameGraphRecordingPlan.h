#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "RHI/Public/Interop/ResourceState.h"

#include <cstdint>
#include <vector>

using RecordingGroupIndex = std::uint32_t;
using RecordingChunkIndex = std::uint32_t;

static constexpr RecordingGroupIndex InvalidRecordingGroupIndex = static_cast<RecordingGroupIndex>(-1);
static constexpr RecordingChunkIndex InvalidRecordingChunkIndex = static_cast<RecordingChunkIndex>(-1);

struct SubmissionOrderKey final
{
	std::uint32_t Batch = 0;
	std::uint32_t Position = 0;

	bool operator==(const SubmissionOrderKey&) const noexcept;
};

struct RecordingResourceState final
{
	FrameGraphResourceHandle Resource = FrameGraphResourceHandle::Invalid();
	ResourceState State = ResourceState::Common;
};

enum class RecordingContextRequirement : std::uint8_t
{
	Coordinator,
	ExclusiveLease,
};

enum class RecordingSerialIslandReason : std::uint8_t
{
	None,
	Callback,
	ExternalProvider,
	Presentation,
};

struct RecordingGroup final
{
	RecordingGroupIndex Index = InvalidRecordingGroupIndex;
	std::uint32_t PassOffset = 0;
	std::uint32_t PassCount = 0;
	ERhiQueueType Queue = ERhiQueueType::Graphics;
	std::vector<RecordingGroupIndex> Prerequisites;
	std::vector<RecordingResourceState> InitialResourceStates;
	std::vector<RecordingResourceState> FinalResourceStates;
	RecordingContextRequirement ContextRequirement = RecordingContextRequirement::Coordinator;
	std::uint32_t EstimatedRecordingCost = 0;
	SubmissionOrderKey SubmissionOrder;
	RecordingSerialIslandReason SerialIslandReason = RecordingSerialIslandReason::Callback;
};

struct RecordingChunk final
{
	RecordingChunkIndex Index = InvalidRecordingChunkIndex;
	RecordingGroupIndex FirstGroup = InvalidRecordingGroupIndex;
	std::uint32_t GroupCount = 0;
	ERhiQueueType Queue = ERhiQueueType::Graphics;
	RecordingContextRequirement ContextRequirement = RecordingContextRequirement::Coordinator;
	std::uint32_t EstimatedRecordingCost = 0;
	SubmissionOrderKey SubmissionOrder;
};

struct RecordingPlan final
{
	static constexpr std::uint32_t TargetParallelChunkCost = 16;
	static constexpr std::uint32_t MinimumParallelRecordingCost = 32;
	static constexpr std::uint32_t MaximumChunksPerSubmissionBatch = 8;

	std::vector<std::uint32_t> Passes;
	std::vector<RecordingGroup> Groups;
	std::vector<RecordingChunk> Chunks;

	void Clear() noexcept;
};
