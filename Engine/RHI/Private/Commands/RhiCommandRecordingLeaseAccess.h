#pragma once

#include "Commands/RhiCommandRecordingLease.h"

#include <utility>

struct RhiCommandRecordingLeaseInitialization final
{
	void* BackendState = nullptr;
	RenderCommandList* CommandList = nullptr;
	ERhiQueueType QueueType = ERhiQueueType::Graphics;
	std::uint32_t FrameSlot = 0;
	RhiCommandRecordingContextId ContextId = {};
	RhiCommandRecordingOwner Owner = {};
	RhiCommandRecordingDescriptorPage DescriptorPage = {};
	RhiSubmissionToken RetirementToken = {};
	void (*Begin)(void*) noexcept = nullptr;
	void (*Close)(void*) noexcept = nullptr;
	void (*Release)(void*, bool) noexcept = nullptr;
	RhiTransientDescriptorRange (*AllocateDescriptors)(void*, std::uint32_t) noexcept = nullptr;
};

struct RhiCommandRecordingLeaseBackendState final
{
	void* State = nullptr;
	RenderCommandList* CommandList = nullptr;
	ERhiQueueType QueueType = ERhiQueueType::Graphics;
	std::uint32_t FrameSlot = 0;
	RhiCommandRecordingContextId ContextId = {};
	RhiCommandRecordingOwner Owner = {};
	bool Closed = false;
};

class RhiCommandRecordingLeaseAccess final
{
  public:
	static RhiCommandRecordingLease Create(const RhiCommandRecordingLeaseInitialization& initialization) noexcept;
	static RhiCommandRecordingLeaseBackendState Consume(RhiCommandRecordingLease&& lease) noexcept;
	static bool Matches(
	    const RhiCommandRecordingLeaseBackendState& state,
	    const RhiCommandRecordingLeaseBackendState& expected) noexcept;

	template <typename Slot, typename RecordingContext, typename SlotState>
	static Slot* ConsumeClosed(
	    RhiCommandRecordingLease&& lease,
	    RecordingContext* recordingContext,
	    SlotState closedState) noexcept
	{
		const RhiCommandRecordingLeaseBackendState leaseState = Consume(std::move(lease));
		auto* const slot = static_cast<Slot*>(leaseState.State);
		if (slot == nullptr || slot->Owner != recordingContext || slot->State != closedState ||
		    !Matches(
		        leaseState,
		        RhiCommandRecordingLeaseBackendState{
		            .State = slot,
		            .CommandList = slot->CommandList.get(),
		            .QueueType = slot->QueueType,
		            .FrameSlot = slot->FrameSlot,
		            .ContextId = RhiCommandRecordingContextId{.Value = slot->ContextIndex},
		            .Owner = slot->RecordingOwner,
		            .Closed = true}))
		{
			return nullptr;
		}

		return slot;
	}
};
