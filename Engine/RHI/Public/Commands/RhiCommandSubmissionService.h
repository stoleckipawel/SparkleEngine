#pragma once

#include "../RHIAPI.h"

#include <cstdint>

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
	virtual void SubmitFrame() noexcept = 0;
	virtual void AdvanceFrameInFlight() noexcept = 0;
	virtual void CloseExecuteAndFlushCurrentFrame() noexcept = 0;
};
