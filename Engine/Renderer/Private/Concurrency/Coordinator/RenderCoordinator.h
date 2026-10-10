#pragma once

#include "Renderer/Public/Viewport/ViewportPresentationSnapshot.h"

#include "Concurrency/Control/RenderThreadCommandQueue.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"
#include "RHI/Public/Device/RhiDeviceLaunch.h"
#include "Renderer/Public/Concurrency/RendererExecutionConfig.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Renderer/Public/UI/UiTextureHandle.h"
#include "Viewport/ViewportCaptureCompletion.h"
#include "Core/Public/Events/ScopedEventHandle.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <array>
#include <cstddef>
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
	RenderCoordinator(Timer& timer, Window& window, RendererExecutionConfig config, RhiDeviceLaunch deviceLaunch);
	~RenderCoordinator() noexcept;

	RenderCoordinator(const RenderCoordinator&) = delete;
	RenderCoordinator& operator=(const RenderCoordinator&) = delete;

	void StageFrameSubmission(RenderFrameSubmission submission);
	void StageUiRenderPacket(UiRenderPacket packet);
	void SubmitRenderingSettings(EngineRenderingSettingsState settings);
	EngineRenderingSettingsState CaptureRenderingSettings();
	CVarControlResult ExecuteConsoleVariables(CVarControlRequest request);
	void SubmitViewportRequest(ViewportRenderRequest request);
	void RenderFrame();

	ViewportPresentationSnapshot GetViewportPresentation() const;
	void ReloadShaders();
	std::uint64_t GetShaderGeneration() const noexcept;
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics();
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId);
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics();
	ViewportCaptureAdmission RequestViewportCapture(ViewportCaptureRequest request);
	bool TryTakeViewportCapture(ViewportCaptureId id, ViewportCaptureReadback& readback);

	ExternalCaptureAdmission RequestExternalCapture(std::uint64_t viewportGeneration) noexcept;

	RendererExecutionMode GetMode() const noexcept { return m_config.Mode; }

private:
	static constexpr std::size_t RenderThreadCommandCapacity = 64;

	static constexpr std::size_t MaximumOutstandingViewportCaptures = 3;

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
	void ProcessThreadedCommand(RenderThreadCommand command);
	void ExecuteThreadedFrame(RenderFrameQueueTicket ticket);
	void SettleAbandonedWork() noexcept;
	void PublishReadState();
	void DispatchControl(RendererExecutionControl control);
	void SubmitThreadCommand(RenderThreadCommandPayload payload);
	template <typename TCommand> RenderControlResult ExecuteSynchronousControl(TCommand command);
	std::uint64_t IssueThreadCommandSequence() noexcept;
	void SubmitResize();
	RendererExecutionContext& GetSerialContext();
	const RendererExecutionContext& GetSerialContext() const;

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	RendererExecutionConfig m_config;
	RhiDeviceLaunch m_deviceLaunch;
	Threading::OwnerThread m_producerOwner{"RenderCoordinator producer"};
	std::unique_ptr<RenderFrameQueue> m_frameQueue;
	std::unique_ptr<RenderThreadCommandQueue> m_threadCommandQueue;
	std::unique_ptr<RendererExecutionContext> m_context;
	std::thread m_renderThread;
	ScopedEventHandle m_resizeHandle;
	std::optional<RenderFrameSubmission> m_pendingSubmission;
	std::optional<UiRenderPacket> m_pendingUi;
	std::uint64_t m_nextThreadCommandSequence = 1;
	std::uint64_t m_lastConsumedThreadCommandSequence = 0;
	mutable std::mutex m_startMutex;
	std::condition_variable m_startedCondition;
	bool m_started = false;
	bool m_startSucceeded = false;
	mutable std::mutex m_readStateMutex;
	ViewportPresentationSnapshot m_publishedViewportPresentation;
	std::atomic<std::uint64_t> m_shaderGeneration{0};
	std::array<ViewportCaptureId, MaximumOutstandingViewportCaptures> m_outstandingViewportCaptures{};
	std::vector<ViewportCaptureCompletion> m_publishedViewportCaptures;
	std::uint64_t m_nextViewportCaptureId = 1;
};
