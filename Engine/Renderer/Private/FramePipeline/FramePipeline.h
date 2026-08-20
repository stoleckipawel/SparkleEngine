#pragma once

#include "Frame/Builders/FrameContextBuilder.h"
#include "Frame/Core/Frame.h"
#include "Frame/Presentation/ViewportDisplaySettings.h"
#include "FramePipeline/FrameExecutionRetirementQueue.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "ShaderData/FrameUniformData.h"
#include "Rendering/RenderFrameSubmission.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"
#include "View/RenderViewState.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class FrameExecutionDiagnostics;
class UiRenderPacketPlayer;
class EditorTextureRegistry;
struct FrameContext;
class FrameGraph;
class RenderCommandList;
class RenderDeviceServices;
class RendererHost;
struct RenderFrameTime;
struct RenderRayTracingFrameBindings;
struct RenderViewInput;

struct FrameResolutionExtents final
{
	RenderViewportExtent Render;
	RenderViewportExtent Output;
};

class FramePipeline final
{
public:
	FramePipeline(RendererHost& rendererHost, bool enableUiRenderPackets) noexcept;
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
	void InitializeUiRendering();
	void InitializeFrameStorage();
	void InitializeFrameContexts();
	void InitializeFrameGraph() noexcept;
	void InitializeFrameGraph(FrameResolutionExtents resolution) noexcept;
	void RefreshFrameExecution(FrameResolutionExtents resolution) noexcept;
	void RebuildFrameExecutionAfterSwapChainDrain(FrameResolutionExtents resolution) noexcept;
	void RetireFrameExecution() noexcept;
	bool ShouldOutputToBackBuffer() const noexcept;
	RenderViewportExtent ResolveOutputExtent() const noexcept;
	FrameResolutionExtents ResolveFrameResolution() const noexcept;
	bool BeginFrame(RenderFrameSubmission& submission) noexcept;
	void PollFrameServices() noexcept;
	bool AcceptFrameSubmission(RenderFrameSubmission& submission) noexcept;
	void ApplyPendingResize() noexcept;
	void RefreshGraphForResolutionAndPresentation() noexcept;
	void RefreshGraphForRenderModes() noexcept;
	void RefreshGraphForImageProvider() noexcept;
	void BeginBackendFrame() noexcept;
	void PollViewportCaptures() noexcept;
	void SetupFrame(const RenderFrameTime& time) noexcept;
	static FrameUniformData BuildFrameUniformData(std::uint64_t frameId, const RenderFrameTime& time) noexcept;
	void UploadPendingSceneTextures(RenderDeviceServices& deviceServices, RenderCommandList& graphicsCommandList);
	void RefreshViewportRenderProducts() noexcept;
	bool BeginViewportEditorTexturePresentation(RenderOutputFlags output) noexcept;
	void EndViewportEditorTexturePresentation(RenderOutputFlags output) noexcept;
	void RenderUiPacket(const UiRenderPacket& packet) noexcept;
	void RenderEditorViewportUi(const UiRenderPacket& packet) noexcept;
	void RenderHostOverlayUi(const UiRenderPacket& packet) noexcept;
	void PlayUiPacket(const UiRenderPacket& packet) noexcept;
	FrameGraphResourceHandle ResolveRenderProductResourceHandle(RenderProductHandle handle) const noexcept;
	void TransitionRenderProduct(RenderProductHandle handle, ResourceState after) noexcept;
	void RecordFrame(const RenderViewInput& viewInput) noexcept;
	FrameContext& PrepareFrameContext(const RenderViewInput& viewInput);
	void UpdateLightingHistory(FrameContext& frame);
	void SetupImageProviderFrame(const FrameContext& frame);
	void BindRayTracingScene(const FrameContext& frame, const RenderRayTracingFrameBindings& rayTracingBindings);
	void BindSkyTexture(const FrameContext& frame);
	void ExecuteFrameGraph(FrameContext& frame);
	void InvalidateViewHistory(RenderViewInvalidationReason reason) noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;

	RendererHost* m_rendererHost = nullptr;
	FrameContextBuilder m_frameContextBuilder;
	std::unique_ptr<FrameGraph> m_frameGraph;
	std::vector<std::unique_ptr<FrameExecutionDiagnostics>> m_frameExecutionDiagnostics;
	std::vector<std::unique_ptr<FrameContext>> m_frameContexts;
	FrameExecutionRetirementQueue m_frameExecutionRetirementQueue;
	RenderViewportExtent m_frameGraphRenderExtent = {};
	RenderViewportExtent m_frameGraphOutputExtent = {};
	RenderViewportExtent m_windowExtent = {};
	FramePresentationTarget m_frameGraphPresentationTarget = FramePresentationTarget::BackBuffer;
	ViewportRenderRequest m_viewportRenderRequest = {};
	ViewportRenderProducts m_viewportRenderProducts = {};
	FrameAssemblyResourceLayout m_frameResources = {};
	FrameUniformData m_frameUniform = {};
	std::optional<std::uint64_t> m_previousReferenceLightingHistoryInvalidationHash;
	std::optional<std::uint64_t> m_previousRestirLightingHistoryInvalidationHash;
	std::uint64_t m_frameId = 0u;
	std::uint64_t m_graphTopologyGeneration = 0u;
	std::unique_ptr<UiRenderPacketPlayer> m_uiRenderPacketPlayer;
	std::unique_ptr<EditorTextureRegistry> m_editorTextureRegistry;
	struct PendingViewportCapture final
	{
		ViewportCaptureId Id;
		RhiCaptureTicket Ticket;
		std::uint64_t SceneGeneration = 0;
		std::uint64_t ProviderGeneration = 0;
	};
	std::vector<std::unique_ptr<PendingViewportCapture>> m_pendingViewportCaptures;
	std::vector<ViewportCaptureReadback> m_completedViewportCaptures;
	bool m_bResizePending = false;
	bool m_windowMinimized = false;
	GBufferMode m_gBufferMode = GBufferMode::Rasterized;
	LightingMode m_lightingMode = LightingMode::RestirPathTraced;
	EngineExposureMeteringMethod m_exposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	ResolvedViewportDisplaySettings m_displaySettings = {};
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
	bool m_ownsUiBackend = false;
};
