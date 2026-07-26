#pragma once

#include "Frame/Builders/PerFrameDataBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "ShaderData/PerFrameConstantBufferData.h"
#include "Rendering/RenderInputFrame.h"
#include "RendererSerialUiCallback.h"
#include "Renderer/Public/Editor/EditorRenderPacket.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

class FrameExecutionDiagnostics;
class EditorRenderPacketPlayer;
class EditorTextureRegistry;
struct FrameContext;
class FrameGraph;
class RendererSystemRoot;
class RenderInputConsumer;
class PersistentRenderGpuScene;
struct TimeInfo;

struct FrameResolutionExtents final
{
	RenderViewportExtent Render;
	RenderViewportExtent Output;
};

class FramePipeline final
{
  public:
	FramePipeline(RendererSystemRoot& systems, bool enableEditorRenderPackets) noexcept;
	~FramePipeline() noexcept;

	FramePipeline(const FramePipeline&) = delete;
	FramePipeline& operator=(const FramePipeline&) = delete;
	FramePipeline(FramePipeline&&) = delete;
	FramePipeline& operator=(FramePipeline&&) = delete;

	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept { m_viewportRenderRequest = request; }
	void SubmitRenderInput(RenderInputFrame input) noexcept;
	void RequestResize(RenderViewportExtent extent, bool minimized) noexcept;
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept { return m_viewportRenderProducts; }

	void RenderSerialUiFrame(
	    const TimeInfo& timing,
	    RendererSerialUiCallback composeUi,
	    void* context) noexcept;
	void OnRender(const TimeInfo& timing, const EditorRenderPacket& editorUi) noexcept;

	bool BeginViewportCapture(
	    ViewportCaptureId id,
	    const ViewportCaptureRequest& request) noexcept;
	std::vector<ViewportCaptureReadback> TakeCompletedViewportCaptures();
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();

  private:
	void InitializeFrameGraph() noexcept;
	void InitializeFrameGraph(FrameResolutionExtents resolution) noexcept;
	void RefreshFrameExecution() noexcept;
	void RefreshFrameExecution(FrameResolutionExtents resolution) noexcept;
	void RebuildFrameExecutionAfterSwapChainDrain(FrameResolutionExtents resolution) noexcept;
	void RetireFrameExecution() noexcept;
	void PollRetiredFrameExecutions() noexcept;
	RhiSubmissionState CaptureLastSubmittedState() const noexcept;
	bool IsSubmissionStateComplete(const RhiSubmissionState& state) const noexcept;
	bool ShouldOutputToBackBuffer() const noexcept;
	RenderViewportExtent ResolveOutputExtent() const noexcept;
	FrameResolutionExtents ResolveFrameResolution() const noexcept;
	void FinalizeRenderInputMetadata(RenderInputFrame& input) const noexcept;
	void BeginFrame() noexcept;
	void PollViewportCaptures() noexcept;
	void SetupFrame(const TimeInfo& timing) noexcept;
	void RefreshViewportRenderProducts() noexcept;
	bool BeginViewportEditorTexturePresentation(
	    RenderOutputFlags output) noexcept;
	void EndViewportEditorTexturePresentation(
	    RenderOutputFlags output) noexcept;
	void RenderEditorPacket(const EditorRenderPacket& packet) noexcept;
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
	struct RetiredFrameExecution final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<FrameGraph> Graph;
		std::vector<std::unique_ptr<FrameContext>> FrameContexts;
	};
	std::vector<RetiredFrameExecution> m_retiredFrameExecutions;
	RenderViewportExtent m_frameGraphRenderExtent = {};
	RenderViewportExtent m_frameGraphOutputExtent = {};
	RenderViewportExtent m_windowExtent = {};
	bool m_frameGraphPresentsToBackBuffer = true;
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	FrameAssemblyResourceLayout m_frameResources = {};
	PerFrameConstantBufferData m_perFrameData = {};
	std::optional<std::uint64_t> m_previousReferenceLightingHistoryInvalidationHash;
	std::optional<std::uint64_t> m_previousRestirLightingHistoryInvalidationHash;
	std::unique_ptr<RenderInputConsumer> m_renderInputConsumer;
	std::unique_ptr<PersistentRenderGpuScene> m_gpuScene;
	std::unique_ptr<EditorRenderPacketPlayer> m_editorRenderPacketPlayer;
	std::unique_ptr<EditorTextureRegistry> m_editorTextureRegistry;
	struct PendingViewportCapture final
	{
		ViewportCaptureId Id;
		RhiCaptureTicket Ticket;
		RenderFrameMetadata Metadata;
	};
	std::vector<std::unique_ptr<PendingViewportCapture>> m_pendingViewportCaptures;
	std::vector<ViewportCaptureReadback> m_completedViewportCaptures;
	bool m_bResizePending = false;
	bool m_windowMinimized = false;
	GBufferMode m_gBufferMode = GBufferMode::Rasterized;
	LightingMode m_lightingMode = LightingMode::RestirPathTraced;
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
	bool m_ownsEditorUiBackend = false;
};
