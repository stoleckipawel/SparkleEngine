#pragma once

#include "Frame/Builders/FrameContextBuilder.h"
#include "Frame/Builders/PerFrameDataBuilder.h"
#include "Frame/Core/Frame.h"
#include "Frame/Presentation/ViewportDisplaySettings.h"
#include "FramePipeline/FrameExecutionRetirementQueue.h"
#include "Providers/RendererImageProviderStack.h"
#include "Resources/History/FrameHistory.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Commands/RhiQueue.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "ShaderData/PerFrameConstantBufferData.h"
#include "Rendering/RenderInputFrame.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
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
class RenderInputConsumer;
class PersistentRenderGpuScene;
class RenderRayTracingScene;
struct RenderFrameDynamicData;
struct TimeInfo;

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
	void SubmitRenderInput(RenderInputFrame input) noexcept;
	void RequestResize(RenderViewportExtent extent, bool minimized) noexcept;
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept { return m_viewportRenderProducts; }

	void OnRender(const TimeInfo& timing, const UiRenderPacket& ui) noexcept;

	bool BeginViewportCapture(ViewportCaptureId id, const ViewportCaptureRequest& request) noexcept;
	std::vector<ViewportCaptureReadback> TakeCompletedViewportCaptures();
	TextureDiagnosticsSnapshot CaptureTextureDiagnostics();

private:
	void InitializeSceneData();
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
	void FinalizeRenderInputMetadata(RenderInputFrame& input) const noexcept;
	void BeginFrame() noexcept;
	void PollFrameServices() noexcept;
	void ConsumeRenderInput() noexcept;
	void ApplyPendingResize() noexcept;
	void RefreshGraphForResolutionAndPresentation() noexcept;
	void RefreshGraphForRenderModes() noexcept;
	void RefreshGraphForImageProvider() noexcept;
	void BeginBackendFrame() noexcept;
	void PollViewportCaptures() noexcept;
	void SetupFrame(const TimeInfo& timing) noexcept;
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
	void RecordFrame() noexcept;
	void ApplyRenderInputHistoryReset(const RenderFrameDynamicData& dynamic) noexcept;
	FrameContext& PrepareFrameContext(const RenderFrameDynamicData& dynamic, RenderRayTracingScene* activeRayTracingScene);
	void UpdateLightingHistory(FrameContext& frame);
	void SetupImageProviderFrame(const FrameContext& frame, const RenderFrameDynamicData& dynamic);
	void BindRayTracingScene(FrameContext& frame, RenderRayTracingScene* activeRayTracingScene);
	void BindSkyTexture(const FrameContext& frame);
	void ExecuteFrameGraph(FrameContext& frame, RenderRayTracingScene* activeRayTracingScene);
	void ResetTemporalState(std::string_view reason) noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;
	FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() noexcept;
	const FrameExecutionDiagnostics& GetCurrentFrameDiagnostics() const noexcept;

	RendererHost* m_rendererHost = nullptr;
	FrameContextBuilder m_frameContextBuilder;
	std::unique_ptr<FrameGraph> m_frameGraph;
	PerFrameDataBuilder m_perFrameDataBuilder;
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
	PerFrameConstantBufferData m_perFrameData = {};
	std::optional<std::uint64_t> m_previousReferenceLightingHistoryInvalidationHash;
	std::optional<std::uint64_t> m_previousRestirLightingHistoryInvalidationHash;
	std::unique_ptr<RenderInputConsumer> m_renderInputConsumer;
	std::unique_ptr<PersistentRenderGpuScene> m_gpuScene;
	std::unique_ptr<UiRenderPacketPlayer> m_uiRenderPacketPlayer;
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
	EngineExposureMeteringMethod m_exposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	ResolvedViewportDisplaySettings m_displaySettings = {};
	ImageProviderGraphKey m_imageProviderFrameGraphKey = {};
	bool m_ownsUiBackend = false;
};
