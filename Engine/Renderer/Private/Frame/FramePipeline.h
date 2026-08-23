#pragma once

#include "Frame/Graph/BuildRenderFrameGraph.h"
#include "Frame/Retirement/FrameExecutionRetirementQueue.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Rendering/RenderFrameSubmission.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"
#include "View/RenderViewState.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

class FrameExecutionDiagnostics;
class UiFrameRenderer;
struct RenderFrame;
class FrameGraph;
class RenderDeviceServices;
class GpuMeshCache;
class RendererImageProviderStack;
class RendererMemoryMonitor;
class RenderPassRuntimeCache;
class RenderScene;
class RenderScenePreparation;
class RenderViewBuilder;
class RenderViewPreparation;
class TaskExecutor;
class TextureCache;
class Window;
struct RenderFrameTime;
struct RenderRayTracingFrameBindings;
struct RenderViewInput;

class FramePipeline final
{
public:
	FramePipeline(
	    Window& window,
	    RenderDeviceServices& deviceServices,
	    RenderPassRuntimeCache& renderPassRuntimeCache,
	    RendererMemoryMonitor& memoryMonitor,
	    GpuMeshCache& gpuMeshCache,
	    TextureCache& textureCache,
	    RenderScenePreparation& renderScenePreparation,
	    RenderViewBuilder& renderViewBuilder,
	    RenderViewPreparation& renderViewPreparation,
	    RenderViewState& renderViewState,
	    RenderScene& renderScene,
	    RendererImageProviderStack& imageProviders,
	    TaskExecutor& taskExecutor,
	    bool enableUiRenderPackets) noexcept;
	~FramePipeline() noexcept;

	FramePipeline(const FramePipeline&) = delete;
	FramePipeline& operator=(const FramePipeline&) = delete;
	FramePipeline(FramePipeline&&) = delete;
	FramePipeline& operator=(FramePipeline&&) = delete;

	void SubmitViewportRenderRequest(ViewportRenderRequest request) noexcept { m_viewportRenderRequest = std::move(request); }
	void RequestResize(RenderViewportExtent extent, bool minimized) noexcept;
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept { return m_viewportRenderProducts; }

	void OnRender(RenderFrameSubmission submission, const RenderFrameTime& time, const UiRenderPacket& ui) noexcept;

	bool BeginViewportCapture(ViewportCaptureId id, const ViewportCaptureRequest& request) noexcept;
	std::vector<ViewportCaptureReadback> TakeCompletedViewportCaptures();
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();

private:
	void InitializeFrameStorage();
	void InitializeRenderFrames();
	void InitializeFrameGraph() noexcept;
	void InitializeFrameGraph(const RenderFrameGraphSettings& settings) noexcept;
	void RefreshFrameExecution(const RenderFrameGraphSettings& settings) noexcept;
	void RebuildFrameExecutionAfterSwapChainDrain(const RenderFrameGraphSettings& settings) noexcept;
	void RetireFrameExecution() noexcept;
	bool ShouldOutputToBackBuffer() const noexcept;
	RenderViewportExtent ResolveOutputExtent() const noexcept;
	RenderFrameGraphSettings ResolveFrameGraphSettings() const noexcept;
	bool BeginFrame(RenderFrameSubmission& submission) noexcept;
	void PollFrameServices() noexcept;
	bool AcceptFrameSubmission(RenderFrameSubmission& submission) noexcept;
	void ApplyPendingResize() noexcept;
	void RefreshGraphForTopology() noexcept;
	void BeginBackendFrame() noexcept;
	void PollViewportCaptures() noexcept;
	void RefreshViewportRenderProducts() noexcept;
	RenderRayTracingFrameBindings PrepareFrame(const RenderViewInput& viewInput, const RenderFrameTime& time);
	void ExecuteFrame(const RenderRayTracingFrameBindings& rayTracingBindings);
	void SubmitAndPresent(const UiRenderPacket& packet) noexcept;
	RenderFrame& PrepareRenderFrame(const RenderViewInput& viewInput, const RenderFrameTime& time);
	void SetupImageProviderFrame(const RenderFrame& frame);
	void InvalidateViewHistory(RenderViewInvalidationReason reason) noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;

	Window& m_window;
	RenderDeviceServices& m_deviceServices;
	RenderPassRuntimeCache& m_renderPassRuntimeCache;
	RendererMemoryMonitor& m_memoryMonitor;
	GpuMeshCache& m_gpuMeshCache;
	TextureCache& m_textureCache;
	RenderScenePreparation& m_renderScenePreparation;
	RenderViewBuilder& m_renderViewBuilder;
	RenderViewPreparation& m_renderViewPreparation;
	RenderViewState& m_renderViewState;
	RenderScene& m_renderScene;
	RendererImageProviderStack& m_imageProviders;
	TaskExecutor& m_taskExecutor;
	std::unique_ptr<FrameGraph> m_frameGraph;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	std::vector<std::unique_ptr<RenderFrame>> m_renderFrames;
	FrameExecutionRetirementQueue m_frameExecutionRetirementQueue;
	RenderFrameGraphSettings m_frameGraphSettings = {};
	RenderViewportExtent m_windowExtent = {};
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	RenderFrameGraphResources m_frameResources = {};
	std::uint64_t m_frameId = 0u;
	std::uint64_t m_graphTopologyGeneration = 0u;
	std::unique_ptr<UiFrameRenderer> m_uiFrameRenderer;
	struct PendingViewportCapture final
	{
		ViewportCaptureId Id;
		RhiCaptureTicket Ticket;
		std::uint64_t SceneGeneration = 0;
		std::uint64_t ProviderGeneration = 0;
	};
	std::vector<std::unique_ptr<PendingViewportCapture>> m_pendingViewportCaptures;
	std::vector<ViewportCaptureReadback> m_completedViewportCaptures;
	bool m_resizePending = false;
	bool m_windowMinimized = false;
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
};
