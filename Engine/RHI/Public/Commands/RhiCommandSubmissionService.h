#pragma once

#include "RhiQueue.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <span>

class RenderCommandList;

class SPARKLE_RHI_API RhiCommandSubmissionService
{
  public:
	virtual ~RhiCommandSubmissionService() noexcept = default;

	virtual void WaitForIdle() noexcept = 0;
	virtual void Flush() noexcept = 0;
	virtual void ResizeSwapChain() noexcept = 0;
	virtual void BeginFrame() noexcept = 0;
	virtual RenderCommandList& GetCurrentGraphicsCommandList() noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual RenderCommandList& BeginCommandList(ERhiQueueType queueType) noexcept = 0;
	virtual RhiSubmissionToken SubmitCommandList(
	    RenderCommandList& commandList,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept = 0;
	virtual void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept = 0;
	virtual void WaitForSubmission(RhiSubmissionToken token) noexcept = 0;
	virtual bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept = 0;
	virtual RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept = 0;
	virtual void SubmitFrame() noexcept = 0;
	virtual void AdvanceFrameInFlight() noexcept = 0;
	virtual void CloseExecuteAndFlushCurrentFrame() noexcept = 0;
};
