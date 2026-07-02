#pragma once

#include "Core/Public/Events/ScopedEventHandle.h"
#include "Frame/Builders/PerFrameDataBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Frame/Core/FrameRenderPath.h"
#include "Frame/RhiFrameConstants.h"
#include "FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/PerFrameConstantBufferData.h"
#include "Renderer/Public/Diagnostics/RendererDiagnosticsSnapshot.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "Viewport/ViewportContracts.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

class FrameExecutionDiagnostics;
struct FrameContext;
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
	RendererFrameTimingDiagnosticsSnapshot CaptureFrameTimingDiagnosticsSnapshot() const;

  private:
	void InitializeFrameGraph() noexcept;
	void InitializeFrameGraph(RenderViewportExtent sceneExtent) noexcept;
	void BindWindowResizeEvent() noexcept;
	void RefreshFrameExecution() noexcept;
	void RefreshFrameExecution(RenderViewportExtent sceneExtent) noexcept;
	bool ShouldOutputToBackBuffer() const noexcept;
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
	void CreateExposureHistoryResources() noexcept;
	void ReleaseExposureHistoryResources() noexcept;
	void BindExposureHistoryFrameGraphResources() noexcept;
	void ResetExposureHistory() noexcept;
	bool HasExposureHistoryResources() const noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;
	void ReportResolvedTimings(std::uint32_t frameIndex, const FrameExecutionDiagnostics& frameDiagnostics) const noexcept;
	void PublishLiveGpuTimings(const std::vector<ResolvedGpuTiming>& resolvedTimers) const noexcept;

	RendererSystemRoot* m_systems = nullptr;
	std::unique_ptr<FrameGraph> m_frameGraph;
	PerFrameDataBuilder m_perFrameDataBuilder;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	std::vector<std::unique_ptr<FrameContext>> m_frameContexts;
	RenderViewportExtent m_frameGraphSceneExtent = {};
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	FrameAssemblyResourceLayout m_frameResources = {};
	PerFrameConstantBufferData m_perFrameData = {};
	RenderSceneSnapshot m_sceneSnapshot = {};
	ScopedEventHandle m_resizeHandle;
	std::array<RhiOwnedResourceHandle, RhiFrameConstants::FramesInFlight> m_exposureHistoryResources = {};
	bool m_bResizePending = false;
	bool m_exposureHistoryValid = false;
	FrameRenderPath m_renderPath = FrameRenderPath::RealtimeDeferred;
	std::uint32_t m_imageProviderFrameGraphKey = 0;
};
