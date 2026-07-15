#pragma once

#include "../Core/RhiBackendSelection.h"
#include "../Commands/RhiCommandSubmissionService.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/RhiExternalFeatureHooks.h"
#include "../RHIAPI.h"
#include "RenderHardwareInterface.h"

#include <cstdint>
#include <memory>

class Window;
class RhiImGuiRenderer;
class RenderDeviceBackendServices;

class SPARKLE_RHI_API RenderDeviceServices final : public RhiCommandSubmissionService
{
  public:
	static std::unique_ptr<RenderDeviceServices> Create(Window& window) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(Window& window, ERhiBackendApi backendApi) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(
	    Window& window,
	    PixelFormat backBufferFormat) noexcept;
	static std::unique_ptr<RenderDeviceServices> Create(
	    Window& window,
	    ERhiBackendApi backendApi,
	    PixelFormat backBufferFormat,
	    RhiExternalFeatureHooks externalFeatureHooks = {}) noexcept;

	~RenderDeviceServices() noexcept;

	RenderDeviceServices(const RenderDeviceServices&) = delete;
	RenderDeviceServices& operator=(const RenderDeviceServices&) = delete;
	RenderDeviceServices(RenderDeviceServices&&) = delete;
	RenderDeviceServices& operator=(RenderDeviceServices&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept;
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	void WaitForIdle() noexcept override;
	void ResizeSwapChain() noexcept;
	void BeginFrame() noexcept;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RenderCommandList& BeginCommandList(ERhiQueueType queueType) noexcept override;
	RhiSubmissionToken SubmitCommandList(
	    RenderCommandList& commandList,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept override;
	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept override;
	void WaitForSubmission(RhiSubmissionToken token) noexcept override;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept override;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept override;
	void SubmitFrame() noexcept;
	void AdvanceFrameInFlight() noexcept;
	void CloseExecuteAndFlushCurrentFrame() noexcept;

  private:
	RenderDeviceServices() noexcept;

	std::unique_ptr<RenderDeviceBackendServices> m_backend;
};
