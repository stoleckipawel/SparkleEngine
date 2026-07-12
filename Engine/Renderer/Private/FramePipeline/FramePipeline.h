#pragma once

#include "Core/Public/Events/ScopedEventHandle.h"
#include "Frame/Builders/PerFrameDataBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Frame/RhiFrameConstants.h"
#include "FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "FramePipeline/ReservoirHistoryResources.h"
#include "Providers/RendererImageProviderStack.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "ShaderData/PerFrameConstantBufferData.h"
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

struct FrameResolutionExtents final
{
	RenderViewportExtent Render;
	RenderViewportExtent Output;
};

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
	ViewportCaptureResult CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept;

  private:
	void InitializeFrameGraph() noexcept;
	void InitializeFrameGraph(FrameResolutionExtents resolution) noexcept;
	void BindWindowResizeEvent() noexcept;
	void RefreshFrameExecution() noexcept;
	void RefreshFrameExecution(FrameResolutionExtents resolution) noexcept;
	bool ShouldOutputToBackBuffer() const noexcept;
	RenderViewportExtent ResolveOutputExtent() const noexcept;
	FrameResolutionExtents ResolveFrameResolution() const noexcept;
	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RefreshViewportRenderProducts() noexcept;
	FrameGraphResourceHandle ResolveRenderProductResourceHandle(RenderProductHandle handle) const noexcept;
	void TransitionRenderProduct(RenderProductHandle handle, ResourceState after) noexcept;
	void RecordFrame() noexcept;
	void CreateExposureHistoryResources() noexcept;
	void ReleaseExposureHistoryResources() noexcept;
	void BindExposureHistoryFrameGraphResources() noexcept;
	void ResetExposureHistory() noexcept;
	bool HasExposureHistoryResources() const noexcept;
	void CreateReferenceLightingHistoryResources() noexcept;
	void ReleaseReferenceLightingHistoryResources() noexcept;
	void BindReferenceLightingHistoryFrameGraphResources() noexcept;
	void ResetReferenceLightingHistory() noexcept;
	bool HasReferenceLightingHistoryResources() const noexcept;
	void CreateDirectLightReservoirHistoryResources() noexcept;
	void ReleaseDirectLightReservoirHistoryResources() noexcept;
	void BindDirectLightReservoirHistoryFrameGraphResources() noexcept;
	void ResetRestirLightingHistory() noexcept;
	bool HasDirectLightReservoirHistoryResources() const noexcept;
	void UpdateLightingHistoryState(const FrameContext& frame) noexcept;
	void CreateRestirIndirectReservoirHistoryResources() noexcept;
	void ReleaseRestirIndirectReservoirHistoryResources() noexcept;
	void BindRestirIndirectReservoirHistoryFrameGraphResources() noexcept;
	bool HasRestirIndirectReservoirHistoryResources() const noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;

	RendererSystemRoot* m_systems = nullptr;
	std::unique_ptr<FrameGraph> m_frameGraph;
	PerFrameDataBuilder m_perFrameDataBuilder;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	std::vector<std::unique_ptr<FrameContext>> m_frameContexts;
	RenderViewportExtent m_frameGraphRenderExtent = {};
	RenderViewportExtent m_frameGraphOutputExtent = {};
	bool m_frameGraphPresentsToBackBuffer = true;
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	FrameAssemblyResourceLayout m_frameResources = {};
	PerFrameConstantBufferData m_perFrameData = {};
	RenderSceneSnapshot m_sceneSnapshot = {};
	ScopedEventHandle m_resizeHandle;
	std::array<RhiOwnedResourceHandle, RhiFrameConstants::FramesInFlight> m_exposureHistoryResources = {};
	std::array<RhiOwnedResourceHandle, RhiFrameConstants::FramesInFlight> m_referenceLightingHistoryResources = {};
	RenderViewportExtent m_referenceLightingHistoryExtent = {};
	ReservoirHistoryResourceSet m_directLightReservoirHistoryResources = {};
	ReservoirHistoryResourceSet m_restirIndirectReservoirHistoryResources = {};
	bool m_bResizePending = false;
	bool m_exposureHistoryValid = false;
	bool m_referenceLightingHistoryValid = false;
	bool m_directLightReservoirHistoryValid = false;
	bool m_restirIndirectReservoirHistoryValid = false;
	GBufferMode m_gBufferMode = GBufferMode::Rasterized;
	LightingMode m_lightingMode = LightingMode::RestirPathTraced;
	std::uint64_t m_referenceLightingSettingsKey = 0u;
	std::uint64_t m_referenceLightingStateKey = 0u;
	std::uint64_t m_restirLightingSettingsKey = 0u;
	std::uint64_t m_restirLightingSceneStateKey = 0u;
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
};
