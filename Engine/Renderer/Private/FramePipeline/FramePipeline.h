#pragma once

#include "Core/Public/Events/ScopedEventHandle.h"
#include "Frame/FrameAssembly.h"
#include "FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Renderer/Public/Diagnostics/RendererDiagnosticsSnapshot.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

class FrameExecutionDiagnostics;
class FrameGraph;
class RendererSystemRoot;
struct ResolvedGpuTiming;
struct RayTracingSceneFrameData;

class FramePipeline final
{
  public:
	explicit FramePipeline(RendererSystemRoot& systems) noexcept;
	~FramePipeline() noexcept;

	FramePipeline(const FramePipeline&) = delete;
	FramePipeline& operator=(const FramePipeline&) = delete;
	FramePipeline(FramePipeline&&) = delete;
	FramePipeline& operator=(FramePipeline&&) = delete;

	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept { m_viewportRenderRequest = request; }
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept { return m_viewportRenderProducts; }

	void PrepareHostFrame() noexcept;
	void RecordHostFrame() noexcept;
	void SubmitHostFrame() noexcept;
	void OnRender() noexcept;

	ViewportPresentationProduct BeginViewportPresentation(RenderOutputFlags output) noexcept;
	void EndViewportPresentation(RenderOutputFlags output) noexcept;
	RhiCaptureResult CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept;
	std::uint32_t GetLastUnresolvedBarrierWarningCount() const noexcept;
	std::uint32_t GetLastMissingExecutionBindingCount() const noexcept;
	std::uint32_t GetCompiledTransientResourceCount() const noexcept;
	std::uint32_t GetCompiledImportedResourceCount() const noexcept;
	std::uint32_t GetCompiledPersistentResourceCount() const noexcept;
	std::uint32_t GetAvailableViewportProductCount() const noexcept;
	bool TryGetLastResolvedGpuTimingMilliseconds(std::string_view label, double& outMilliseconds) const noexcept;
	RendererFrameTimingDiagnosticsSnapshot CaptureFrameTimingDiagnosticsSnapshot() const;

  private:
	void InitializeFrameGraph() noexcept;
	void InitializeFrameGraph(RenderViewportExtent sceneExtent) noexcept;
	void BindWindowResizeEvent() noexcept;
	void RefreshFrameExecution() noexcept;
	void RefreshFrameExecution(RenderViewportExtent sceneExtent) noexcept;
	bool ShouldPresentSceneToBackBuffer() const noexcept;
	RenderViewportExtent ResolveSceneExtent() const noexcept;
	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RefreshViewportRenderProducts() noexcept;
	FrameGraphResourceHandle ResolveRenderProductResourceHandle(RenderProductHandle handle) const noexcept;
	NativeResourceHandle ResolveRenderProductResource(RenderProductHandle handle) const noexcept;
	void TransitionRenderProduct(RenderProductHandle handle, ResourceState after) noexcept;
	void RecordFrame() noexcept;
	void BindRayTracingFrameGraphResources(const RayTracingSceneFrameData& rayTracingScene) noexcept;
	void ClearRayTracingFrameGraphResources() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;
	void ReportResolvedTimings(std::uint32_t frameIndex, const FrameExecutionDiagnostics& frameDiagnostics) const noexcept;
	void PublishLiveGpuTimings(const std::vector<ResolvedGpuTiming>& resolvedTimers) const noexcept;

	RendererSystemRoot* m_systems = nullptr;
	std::unique_ptr<FrameGraph> m_frameGraph;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	RenderViewportExtent m_frameGraphSceneExtent = {};
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	FrameAssemblyResourceLayout m_frameResources = {};
	RenderSceneSnapshot m_sceneSnapshot = {};
	ScopedEventHandle m_resizeHandle;
	bool m_bResizePending = false;
};
