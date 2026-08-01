#pragma once

#include "Commands/RhiCommandRecordingLease.h"

struct RhiCommandRecordingLeaseInitialization final
{
	void* BackendState = nullptr;
	RenderCommandList* CommandList = nullptr;
	ERhiQueueType QueueType = ERhiQueueType::Graphics;
	std::uint32_t FrameSlot = 0;
	RhiCommandRecordingContextId ContextId = {};
	RhiCommandRecordingOwner Owner = {};
	RhiCommandRecordingUploadPage UploadPage = {};
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
};
