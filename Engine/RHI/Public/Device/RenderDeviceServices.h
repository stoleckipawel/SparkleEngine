#pragma once

#include "../Core/RhiBackendSelection.h"
#include "../Commands/RhiCommandSubmissionService.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/RhiD3D12InterposerHooks.h"
#include "../RHIAPI.h"
#include "RenderHardwareInterface.h"

#include <cstdint>
#include <memory>
#include <string_view>

class Window;
class RhiImGuiRenderer;
class RenderDeviceServicesState;
struct RhiPresentationConfiguration;

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
	    RhiD3D12InterposerHooks d3d12InterposerHooks = {}) noexcept;

	~RenderDeviceServices() noexcept;

	RenderDeviceServices(const RenderDeviceServices&) = delete;
	RenderDeviceServices& operator=(const RenderDeviceServices&) = delete;
	RenderDeviceServices(RenderDeviceServices&&) = delete;
	RenderDeviceServices& operator=(RenderDeviceServices&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept;
	RenderHardwareInterface& GetRenderHardwareInterface() noexcept;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	void SettleForShutdown() noexcept;
	void ResizeSwapChain() noexcept;
	void BeginFrame(std::uint64_t frameId) noexcept;
	void PrepareCommandRecording() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RenderCommandList& BeginCurrentGraphicsCommandList() noexcept override;
	RhiCommandRecordingLease AcquireCommandRecordingLease(ERhiQueueType queueType, RhiCommandRecordingOwner owner = {}) noexcept override;
	RhiCommandRecordingLease TakeCurrentGraphicsCommandRecordingLease() noexcept override;
	RhiSubmissionToken SubmitCommandRecordingLease(
	    RhiCommandRecordingLease&& lease,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept override;
	RhiSubmissionToken SubmitCommandRecordingBatch(
	    std::span<RhiCommandRecordingLease> leases,
	    std::span<const RhiSubmissionToken> waitTokens = {}) noexcept override;
	RhiSubmissionToken SubmitCurrentGraphicsCommandList(std::span<const RhiSubmissionToken> waitTokens = {}) noexcept override;
	void QueueWait(ERhiQueueType waitQueue, RhiSubmissionToken executionToken) noexcept override;
	void WaitForSubmission(RhiSubmissionToken token) noexcept override;
	bool IsSubmissionComplete(RhiSubmissionToken token) const noexcept override;
	RhiSubmissionToken GetLastSubmittedToken(ERhiQueueType queueType) const noexcept override;
	void SubmitFrame(std::uint64_t frameId) noexcept;
	void AdvanceFrameInFlight() noexcept;

  private:
	RenderDeviceServices() noexcept;
	static void FailCreation(std::string_view message) noexcept;
	static void FailUnsupportedBackend(ERhiBackendApi api) noexcept;
	static void ValidateBackBufferFormat(PixelFormat backBufferFormat) noexcept;
	static RhiPresentationConfiguration ResolvePresentationConfiguration() noexcept;

	std::unique_ptr<RenderDeviceServicesState> m_state;
};
