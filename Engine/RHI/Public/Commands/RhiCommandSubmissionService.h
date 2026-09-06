#pragma once

#include "../RHIAPI.h"
#include "RhiCommandRecordingLease.h"

#include <cstdint>
#include <span>

class RenderCommandList;

class SPARKLE_RHI_API RhiCommandSubmissionService
{
public:
	virtual ~RhiCommandSubmissionService() noexcept;
	RhiCommandSubmissionService(const RhiCommandSubmissionService&) = delete;
	RhiCommandSubmissionService& operator=(const RhiCommandSubmissionService&) = delete;
	RhiCommandSubmissionService(RhiCommandSubmissionService&&) = delete;
	RhiCommandSubmissionService& operator=(RhiCommandSubmissionService&&) = delete;

	virtual void PrepareCommandRecording() noexcept = 0;
	virtual RenderCommandList& GetCurrentGraphicsCommandList() noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual RenderCommandList& BeginCurrentGraphicsCommandList() noexcept = 0;
	virtual RhiCommandRecordingLease AcquireCommandRecordingLease(
	    ERhiQueueType queueType,
	    RhiCommandRecordingOwner owner = {}) noexcept = 0;
	virtual RhiCommandRecordingLease TakeCurrentGraphicsCommandRecordingLease() noexcept = 0;
	virtual RhiSubmissionToken SubmitCommandRecordingLease(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept = 0;
	virtual RhiSubmissionToken SubmitCommandRecordingBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept = 0;
	virtual RhiSubmissionToken SubmitCurrentGraphicsCommandList(std::span<const RhiSubmissionToken> waitTokens = {}) noexcept = 0;
	virtual void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept = 0;
	virtual void WaitForSubmission(RhiSubmissionToken token) noexcept = 0;
	virtual bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept = 0;
	virtual RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept = 0;

protected:
	RhiCommandSubmissionService() noexcept = default;
};
