#pragma once

#include "Core/Public/Events/ScopedEventHandle.h"
#include "Frame/Builders/PerFrameDataBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "ShaderData/PerFrameConstantBufferData.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

class FrameExecutionDiagnostics;
struct FrameContext;
class FrameGraph;
class RendererSystemRoot;

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
	void ResetTemporalState(std::string_view reason) noexcept;
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
	std::optional<std::uint64_t> m_previousReferenceLightingHistoryInvalidationHash;
	std::optional<std::uint64_t> m_previousRestirLightingHistoryInvalidationHash;
	RenderSceneSnapshot m_sceneSnapshot = {};
	ScopedEventHandle m_resizeHandle;
	bool m_bResizePending = false;
	GBufferMode m_gBufferMode = GBufferMode::Rasterized;
	LightingMode m_lightingMode = LightingMode::RestirPathTraced;
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
};
