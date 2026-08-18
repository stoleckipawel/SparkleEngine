#pragma once

#include "Concurrency/Control/RenderControlCommandQueue.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"
#include "Host/RendererBackendConfiguration.h"
#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"
#include "Core/Public/Events/ScopedEventHandle.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

class RendererExecutionContext;
class Timer;
class Window;

class RenderCoordinator final
{
public:
	RenderCoordinator(Timer& timer, Window& window, RendererExecutionConfig config, RendererBackendConfiguration backendConfiguration);
	~RenderCoordinator() noexcept;

	RenderCoordinator(const RenderCoordinator&) = delete;
	RenderCoordinator& operator=(const RenderCoordinator&) = delete;

	void StageFrameSubmission(RenderFrameSubmission submission);
	void StageUiRenderPacket(UiRenderPacket packet);
	void SubmitRenderingSettings(EngineRenderingSettingsState settings);
	void SubmitViewportRequest(ViewportRenderRequest request);
	void RenderFrame();

	ViewportRenderProducts GetViewportRenderProducts() const;
	void ReloadCookedShaders();
	std::uint64_t GetShaderPackageGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics();
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId);
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics();
	ViewportCaptureId RequestViewportCapture(ViewportCaptureRequest request);
	bool TryTakeViewportCapture(ViewportCaptureReadback& readback);

	RendererExecutionMode GetMode() const noexcept { return m_config.Mode; }

private:
	static constexpr std::size_t RenderControlCapacity = 64;

	template <typename TResult> static TResult ExtractControlResult(RenderControlResult result);

	void Initialize();
	void InitializeSerial();
	void InitializeThreaded();
	void StartRenderThread();
	bool WaitForRenderThreadStart();
	void HandleRenderThreadStartFailure();
	RenderExecutionRequest TakePendingExecutionRequest();
	void ExecuteSerialFrame();
	void SubmitThreadedFrame();
	void RenderThreadMain();
	void ProcessThreadedCommand(RenderControlCommand command);
	void ExecuteThreadedFrame(RenderFrameQueueTicket ticket);
	void SettleAbandonedWork() noexcept;
	void PublishReadState();
	void SubmitControl(RenderControlPayload payload);
	template <typename TCommand> RenderControlResult SubmitSynchronousControl(TCommand command);
	std::uint64_t IssueControlSequence() noexcept;
	void SubmitResize();
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
	std::optional<RenderFrameSubmission> m_pendingSubmission;
	std::optional<UiRenderPacket> m_pendingUi;
	std::uint64_t m_nextControlSequence = 1;
	std::uint64_t m_lastConsumedControlSequence = 0;
	mutable std::mutex m_startMutex;
	std::condition_variable m_startedCondition;
	bool m_started = false;
	bool m_startSucceeded = false;
	mutable std::mutex m_readStateMutex;
	ViewportRenderProducts m_publishedViewportProducts;
	std::atomic<std::uint64_t> m_shaderPackageGeneration{0};
	std::vector<ViewportCaptureReadback> m_publishedViewportCaptures;
	std::uint64_t m_nextViewportCaptureId = 1;
};
