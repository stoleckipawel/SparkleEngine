#pragma once

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Frame/Retirement/FrameExecutionRetirementQueue.h"
#include "Providers/ImageProviderGraphKey.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

class FrameExecutionDiagnostics;
class UiFrameRenderer;
struct RenderFrame;
struct RenderFrameSubmission;
class FrameGraph;
class FrameGraphBuilder;
class RenderDeviceServices;
class GpuMeshCache;
class RendererExecutionContext;
class RendererHost;
class RendererImageProviderStack;
class RendererMemoryMonitor;
class RenderPassRuntimeCache;
class ReferencePathTracer;
class RenderScene;
class RenderScenePreparation;
class RenderViewPreparation;
class TaskExecutor;
class TaskScope;
class TextureCache;
class ViewportCaptureService;
class Window;
struct RenderFrameTime;
struct RenderViewInput;
struct UiRenderPacket;
struct MeshDiagnosticsSnapshot;
struct MeshPreviewGeometry;
enum class RenderViewInvalidationReason : std::uint32_t;

class FramePipeline final
{
public:
	~FramePipeline() noexcept;

	FramePipeline(const FramePipeline&) = delete;
	FramePipeline& operator=(const FramePipeline&) = delete;
	FramePipeline(FramePipeline&&) = delete;
	FramePipeline& operator=(FramePipeline&&) = delete;

private:
	friend class RendererExecutionContext;
	friend class RendererHost;

	FramePipeline(
	    Window& window,
	    RenderDeviceServices& deviceServices,
	    RenderPassRuntimeCache& renderPassRuntimeCache,
	    RendererMemoryMonitor& memoryMonitor,
	    TaskExecutor& taskExecutor,
	    TaskScope& applicationTaskScope,
	    bool enableUiRenderPackets) noexcept;

	void SubmitViewportRenderRequest(ViewportRenderRequest request) noexcept { m_viewportRenderRequest = std::move(request); }
	void RequestResize(RenderViewportExtent extent, bool minimized) noexcept;
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept { return m_viewportRenderProducts; }

	void OnRender(RenderFrameSubmission submission, const RenderFrameTime& time, const UiRenderPacket& ui) noexcept;

	bool BeginViewportCapture(ViewportCaptureId id, const ViewportCaptureRequest& request) noexcept;
	std::vector<ViewportCaptureReadback> TakeCompletedViewportCaptures();
	MeshDiagnosticsSnapshot CaptureMeshDiagnostics() const;
	MeshPreviewGeometry CaptureMeshPreview(std::uintptr_t meshRuntimeId) const;
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();
	void InitializeFrameStorage();
	void InitializeRenderFrames();
	RenderFrameGraphResources BuildRenderFrameGraph(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings);
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
	void PrepareFrame(const RenderViewInput& viewInput, const RenderFrameTime& time);
	void ExecuteFrame();
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
	TaskExecutor& m_taskExecutor;
	std::unique_ptr<GpuMeshCache> m_gpuMeshCache;
	std::unique_ptr<TextureCache> m_textureCache;
	std::unique_ptr<RenderScenePreparation> m_renderScenePreparation;
	std::unique_ptr<RenderViewPreparation> m_renderViewPreparation;
	std::unique_ptr<RenderViewState> m_renderViewState;
	std::unique_ptr<RenderScene> m_renderScene;
	std::unique_ptr<RendererImageProviderStack> m_imageProviders;
	std::unique_ptr<FrameGraph> m_frameGraph;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	std::vector<std::unique_ptr<RenderFrame>> m_renderFrames;
	FrameExecutionRetirementQueue m_frameExecutionRetirementQueue;
	RenderFrameGraphSettings m_frameGraphSettings = {};
	GBufferAlgorithm m_builtGBufferAlgorithm = GBufferAlgorithm::Rasterized;
	std::uint64_t m_builtRayTracingGraphGeneration = 0u;
	std::uint64_t m_builtShaderGeneration = 0u;
	RenderViewportExtent m_windowExtent = {};
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	RenderFrameGraphResources m_frameResources = {};
	std::uint64_t m_frameId = 0u;
	std::uint64_t m_graphTopologyGeneration = 0u;
	std::unique_ptr<UiFrameRenderer> m_uiFrameRenderer;
	std::unique_ptr<ViewportCaptureService> m_viewportCaptureService;
	std::unique_ptr<ReferencePathTracer> m_referencePathTracer;
	bool m_resizePending = false;
	bool m_windowMinimized = false;
	bool m_frameGraphExecutable = true;
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
};
