#pragma once

#include "Concurrency/Control/RenderControlCommandQueue.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"
#include "Host/RendererBackendConfiguration.h"
#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"
#include "RendererSerialUiCallback.h"
#include "Core/Public/Events/ScopedEventHandle.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

class RendererExecutionContext;
class RhiImGuiRenderer;
class Timer;
class Window;

class RenderCoordinator final
{
  public:
	RenderCoordinator(
	    Timer& timer,
	    Window& window,
	    RendererExecutionConfig config,
	    RendererBackendConfiguration backendConfiguration);
	~RenderCoordinator() noexcept;

	RenderCoordinator(const RenderCoordinator&) = delete;
	RenderCoordinator& operator=(const RenderCoordinator&) = delete;

	void StageRenderInput(RenderInputFrame input);
	void SubmitViewportRequest(const ViewportRenderRequest& request);
	void RenderFrame();

	void RenderSerialUiFrame(RendererSerialUiCallback composeUi, void* context);

	ViewportRenderProducts GetViewportRenderProducts() const;
	RhiImGuiRenderer& GetSerialImGuiRenderer();
	CookedShaderReloadResult ReloadCookedShaders();
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics();
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId);
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics();
	void WaitForIdle();
	void BeginSerialHostPresentation(const float clearColor[4]);
	void BeginSerialHostOverlayPresentation();
	void EndSerialHostPresentation();
	ViewportPresentationProduct BeginSerialViewportPresentation(RenderOutputFlags output);
	void EndSerialViewportPresentation(RenderOutputFlags output);
	ViewportCaptureResult CaptureViewportProductToBmp(const ViewportCaptureRequest& request);

	RendererExecutionMode GetMode() const noexcept { return m_config.Mode; }

  private:
	static constexpr std::size_t RenderControlCapacity = 64;

	template <typename TResult>
	static TResult ExtractControlResult(RenderControlResult result);

	void Initialize();
	void InitializeSerial();
	void InitializeThreaded();
	void RenderThreadMain();
	void ProcessThreadedCommand(RenderControlCommand command);
	void ExecuteThreadedFrame(RenderFrameQueueTicket ticket);
	void SettleAbandonedWork() noexcept;
	void PublishReadState();
	bool SubmitControl(RenderControlPayload payload);
	RenderControlResult SubmitSynchronousControl(RenderControlPayload payload, const std::shared_ptr<RenderControlCompletion>& completion);
	void SubmitResize();
	void StageInputInSerialContext();
	RendererExecutionContext& GetSerialContext();
	const RendererExecutionContext& GetSerialContext() const;

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	RendererExecutionConfig m_config;
	RendererBackendConfiguration m_backendConfiguration;
	Threading::OwnerThread m_producerOwner{"RenderCoordinator producer"};
	std::unique_ptr<RenderFrameQueue> m_frameQueue;
	std::unique_ptr<RenderControlCommandQueue> m_controlQueue;
	std::unique_ptr<RendererExecutionContext> m_context;
	std::thread m_renderThread;
	ScopedEventHandle m_resizeHandle;
	std::optional<RenderInputFrame> m_pendingInput;
	std::uint64_t m_nextControlSequence = 1;
	std::uint64_t m_lastConsumedControlSequence = 0;
	mutable std::mutex m_startMutex;
	std::condition_variable m_startedCondition;
	bool m_started = false;
	bool m_startSucceeded = false;
	mutable std::mutex m_readStateMutex;
	ViewportRenderProducts m_publishedViewportProducts;
	std::atomic<std::uint64_t> m_shaderPackageGeneration{0};
};
