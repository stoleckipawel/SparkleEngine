#pragma once

#include "Commands/RhiQueue.h"

#include <cstdint>
#include <span>

class RenderCommandList;
class RenderHardwareInterface;
class RhiImGuiRenderer;

class RenderDeviceBackendServices
{
  public:
	virtual ~RenderDeviceBackendServices() noexcept = default;

	virtual RenderHardwareInterface& GetRenderHardwareInterface() noexcept = 0;
	virtual const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept = 0;
	virtual RhiImGuiRenderer& GetImGuiRenderer() noexcept = 0;
	virtual void WaitForIdle() noexcept = 0;
	virtual void ResizeSwapChain() noexcept = 0;
	virtual void BeginFrame() noexcept = 0;
	virtual RenderCommandList& GetCurrentGraphicsCommandList() noexcept = 0;
	virtual RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept = 0;
	virtual RenderCommandList& BeginCommandList(ERhiQueueType queueType) noexcept = 0;
	virtual RhiSubmissionToken SubmitCommandList(
	    RenderCommandList& commandList,
	    std::span<const RhiSubmissionToken> waitTokens) noexcept = 0;
	virtual void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept = 0;
	virtual void WaitForSubmission(RhiSubmissionToken token) noexcept = 0;
	virtual bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept = 0;
	virtual RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept = 0;
	virtual void SubmitFrame() noexcept = 0;
	virtual void AdvanceFrameInFlight() noexcept = 0;
	virtual void CloseExecuteAndFlushCurrentFrame() noexcept = 0;
};
