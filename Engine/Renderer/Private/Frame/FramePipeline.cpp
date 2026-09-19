#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "UI/UiFrameRenderer.h"
#include "Frame/RenderFrame.h"
#include "Frame/Graph/ExecuteRenderFrameGraph.h"
#include "Frame/Graph/RenderProductGraphHandle.h"
#include "Frame/RenderFrameTime.h"
#include "FrameGraph/FrameGraph.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSession.h"
#include "Resources/History/FrameHistory.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "Providers/ImageProviderFrameInput.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Rendering/RenderFrameSubmission.h"
#include "Scene/Preparation/RenderScenePreparation.h"
#include "Scene/RenderScene.h"
#include "Meshes/GpuMeshCache.h"
#include "Textures/TextureCache.h"
#include "View/RenderViewBuilder.h"
#include "View/RenderViewPreparation.h"
#include "View/RenderViewState.h"
#include "Viewport/ViewportCaptureService.h"
#include "Viewport/ViewportRenderProductPublication.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(
    Window& window,
    RenderDeviceServices& deviceServices,
    RenderPassRuntimeCache& renderPassRuntimeCache,
    RendererMemoryMonitor& memoryMonitor,
    TaskExecutor& taskExecutor,
    TaskScope& assetTaskParentScope,
    bool enableUiRenderPackets) noexcept :
    m_window(window),
    m_deviceServices(deviceServices),
    m_renderPassRuntimeCache(renderPassRuntimeCache),
    m_memoryMonitor(memoryMonitor),
    m_taskExecutor(taskExecutor)
{
	RenderHardwareInterface& renderHardwareInterface = m_deviceServices.GetRenderHardwareInterface();
	m_gpuMeshCache = std::make_unique<GpuMeshCache>(renderHardwareInterface, m_deviceServices, m_taskExecutor, assetTaskParentScope);
	m_textureCache = std::make_unique<TextureCache>(
	    renderHardwareInterface.GetResourceService(),
	    renderHardwareInterface.GetDescriptorService(),
	    renderHardwareInterface.GetUploadService(),
	    m_deviceServices,
	    m_taskExecutor,
	    assetTaskParentScope);
	m_renderScenePreparation = std::make_unique<RenderScenePreparation>(m_taskExecutor, *m_gpuMeshCache, *m_textureCache);
	m_renderViewPreparation = std::make_unique<RenderViewPreparation>(m_taskExecutor);
	m_renderViewState = std::make_unique<RenderViewState>();
	m_renderScene = std::make_unique<RenderScene>(
	    &m_deviceServices,
	    *m_gpuMeshCache,
	    *m_textureCache,
	    renderHardwareInterface,
	    BuildRayTracingCapabilityReport(renderHardwareInterface.GetCapabilities()));
	m_imageProviders = std::make_unique<RendererImageProviderStack>(renderHardwareInterface, m_deviceServices);
	m_referencePathTracerSession = std::make_unique<ReferencePathTracerSession>(
	    m_deviceServices,
	    m_memoryMonitor,
	    m_renderScene->GetRayTracingScene());
	m_uiFrameRenderer = std::make_unique<UiFrameRenderer>(m_deviceServices, enableUiRenderPackets);
	m_viewportCaptureService = std::make_unique<ViewportCaptureService>(m_deviceServices);
	m_windowExtent = {static_cast<std::uint32_t>(m_window.GetWidth()), static_cast<std::uint32_t>(m_window.GetHeight())};

	InitializeFrameStorage();
	InitializeFrameGraph();
}

void FramePipeline::InitializeFrameStorage()
{
	const std::uint32_t maximumFramesInFlight =
	    m_deviceServices.GetRenderHardwareInterface().GetCapabilities().Presentation.MaximumFramesInFlight;
	m_frameExecutionDiagnostics.resize(maximumFramesInFlight);
	InitializeRenderFrames();

	RenderDiagnostics& backendDiagnostics = m_deviceServices.GetRenderHardwareInterface().GetDiagnostics();
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}
}

void FramePipeline::InitializeRenderFrames()
{
	m_renderFrames.clear();
	m_renderFrames.resize(m_deviceServices.GetRenderHardwareInterface().GetCapabilities().Presentation.MaximumFramesInFlight);
	for (std::unique_ptr<RenderFrame>& renderFrame : m_renderFrames)
	{
		renderFrame = std::make_unique<RenderFrame>();
	}
}

FramePipeline::~FramePipeline() noexcept = default;

UiTextureHandle FramePipeline::GetViewportPresentationTexture() const noexcept
{
	return m_uiFrameRenderer->GetViewportTexture();
}

bool FramePipeline::BeginViewportCapture(ViewportCaptureId id, const ViewportCaptureRequest& request) noexcept
{
	return m_viewportCaptureService->BeginCapture(
	    id,
	    request,
	    m_viewportRenderProducts,
	    *m_frameGraph,
	    m_frameId,
	    m_renderScene->GetSceneGeneration(),
	    m_imageProviders->GetGeneration());
}

std::vector<ViewportCaptureCompletion> FramePipeline::TakeCompletedViewportCaptures()
{
	return m_viewportCaptureService->TakeCompletedCaptures();
}

MeshDiagnosticsSnapshot FramePipeline::CaptureMeshDiagnostics() const
{
	return MeshDiagnosticsCollector::Capture(*m_renderScene, m_gpuMeshCache.get());
}

MeshPreviewGeometry FramePipeline::CaptureMeshPreview(std::uintptr_t meshRuntimeId) const
{
	return MeshDiagnosticsCollector::CapturePreview(*m_renderScene, meshRuntimeId);
}

TextureDiagnosticsSnapshot FramePipeline::CaptureTextureDiagnostics()
{
	return m_textureCache->CaptureDiagnosticsSnapshot(
	    [this](std::uint64_t nativeTextureId) { return m_uiFrameRenderer->RegisterUiTexture(nativeTextureId); });
}

void FramePipeline::RequestResize(RenderViewportExtent extent, bool minimized) noexcept
{
	m_windowExtent = extent;
	m_windowMinimized = minimized;
	m_resizePending = true;
}

void FramePipeline::OnRender(RenderFrameSubmission submission, const RenderFrameTime& time, const UiRenderPacket& ui) noexcept
{
	if (!BeginFrame(submission))
	{
		return;
	}
	PrepareFrame(submission.View, time);
	ExecuteFrame();
	SubmitAndPresent(ui);
}

bool FramePipeline::BeginFrame(RenderFrameSubmission& submission) noexcept
{
	PollFrameServices();
	if (!AcceptFrameSubmission(submission))
	{
		return false;
	}
	ApplyPendingResize();
	RefreshGraphForTopology();
	BeginBackendFrame();
	return true;
}

void FramePipeline::PollFrameServices() noexcept
{
	m_viewportCaptureService->Poll();
	m_frameExecutionRetirementQueue.Poll(m_deviceServices);
	m_imageProviders->PollRetiredGenerations();
	m_renderPassRuntimeCache.PollRetiredGenerations();
	m_textureCache->PollResidency();
	m_gpuMeshCache->PollResidency();
	m_renderScene->PromoteResidentGpuMeshes();
}

bool FramePipeline::AcceptFrameSubmission(RenderFrameSubmission& submission) noexcept
{
	if (submission.FrameId <= m_frameId)
	{
		return false;
	}

	const bool sceneReset = submission.Scene.Structural.ResetScene;
	if (!m_renderScene->Apply(submission.Scene.Structural, std::move(submission.Scene.Dynamic)))
	{
		return false;
	}

	m_frameId = submission.FrameId;
	if (sceneReset)
	{
		m_textureCache->UnloadSceneTextures();
		InvalidateViewHistory(RenderViewInvalidationReason::SceneGeneration);
	}
	return true;
}

void FramePipeline::BeginBackendFrame() noexcept
{
	RenderDeviceServices& deviceServices = m_deviceServices;
	deviceServices.BeginFrame(m_frameId);
	m_uiFrameRenderer->BeginFrame();

	m_memoryMonitor.Tick(m_frameId);
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
}

void FramePipeline::PrepareFrame(const RenderViewInput& viewInput, const RenderFrameTime& time)
{
	const RenderFrameGraphSettings viewportSettings =
	    m_frameGraphSettings.OutputExtent.IsValid() && m_frameGraphSettings.RenderExtent.IsValid() ? m_frameGraphSettings
	                                                                                               : ResolveFrameGraphSettings();
	RenderCommandList& graphicsCommandList = m_deviceServices.GetCurrentGraphicsCommandList();
	m_gpuMeshCache->UploadReadyMeshes(graphicsCommandList);
	m_textureCache->UpdateSceneTextures(m_renderScene->GetTextures(), m_deviceServices);

	RenderFrame& frame = PrepareRenderFrame(viewInput, time);
	PublishViewportRenderProducts(
	    m_viewportRenderProducts,
	    m_viewportRenderRequest,
	    m_frameResources.ViewportProducts,
	    viewportSettings.RenderExtent,
	    viewportSettings.OutputExtent);
	UpdateFrameHistory(*m_frameGraph, m_frameResources.History, frame.PreparedScene, frame.View, *m_renderViewState, *m_imageProviders);
	SetupImageProviderFrame(frame);
	frame.RayTracingBindings = m_renderScene->PrepareRayTracingFrame(frame.PreparedScene, frame.View.rayTracingPlan);
}

void FramePipeline::ExecuteFrame()
{
	if (!m_frameGraphExecutable)
	{
		return;
	}
	const std::uint32_t frameIndex = m_deviceServices.GetRenderHardwareInterface().GetCurrentFrameIndex();
	const RenderFrame& frame = *m_renderFrames[frameIndex];
	ExecuteRenderFrameGraph(*m_frameGraph, m_frameResources, frame, m_deviceServices, GetCurrentFrameDiagnostics(), m_taskExecutor);
}

RenderFrame& FramePipeline::PrepareRenderFrame(const RenderViewInput& viewInput, const RenderFrameTime& time)
{
	const std::uint32_t frameIndex = m_deviceServices.GetRenderHardwareInterface().GetCurrentFrameIndex();
	std::unique_ptr<RenderFrame>& frameSlot = m_renderFrames[frameIndex];
	RenderFrame& frame = *frameSlot;
	RenderScene& scene = *m_renderScene;
	frame.Identity = RenderFrameIdentity{
	    .FrameId = m_frameId,
	    .SceneGeneration = scene.GetSceneGeneration(),
	    .ShaderGeneration = m_renderPassRuntimeCache.GetShaderGeneration(),
	    .ImageProviderGeneration = m_imageProviders->GetGeneration()};
	frame.Time = time;
	frame.FrameInFlightIndex = frameIndex;

	m_renderScenePreparation->Execute(scene, frame.PreparedScene);
	BuildRenderView(
	    frame.View,
	    *m_renderViewState,
	    RenderViewBuildRequest{
	        .Input = viewInput,
	        .ViewportRequest = m_viewportRenderRequest,
	        .RenderExtent = m_frameGraphSettings.RenderExtent,
	        .OutputExtent = m_frameGraphSettings.OutputExtent,
	        .FrameId = frame.Identity.FrameId,
	        .SceneGeneration = frame.Identity.SceneGeneration,
	        .ShaderGeneration = frame.Identity.ShaderGeneration,
	        .ImageProviderGeneration = frame.Identity.ImageProviderGeneration,
	        .GraphTopologyGeneration = m_graphTopologyGeneration});
	m_renderViewPreparation->Prepare(frame.PreparedScene, frame.View, *m_renderViewState);
	frame.PreparedScene.gpuBindings = &scene.UpdateGpuScene(frame.PreparedScene, frame.View, frame.FrameInFlightIndex);
	m_frameGraphExecutable = m_referencePathTracerSession->PrepareFrame(
	    frame,
	    m_viewportRenderRequest.RenderAction,
	    m_viewportRenderRequest.RenderActionSequence,
	    m_frameResources.ViewportProducts,
	    *m_frameGraph);
	return *frameSlot;
}

void FramePipeline::SetupImageProviderFrame(const RenderFrame& frame)
{
	m_imageProviders->SetupFrame(
	    ImageProviderFrameInput{
	        .RenderExtent = frame.View.renderExtent,
	        .OutputExtent = frame.View.outputExtent,
	        .FrameId = frame.Identity.FrameId,
	        .ProviderGeneration = frame.Identity.ImageProviderGeneration,
	        .Camera = frame.View.cameraUniform,
	        .Temporal = frame.View.temporalUniform,
	        .ResetHistory = frame.View.temporalUniform.HistoryValid == 0u},
	    m_frameGraphSettings.ImagePipeline);
}

void FramePipeline::SubmitAndPresent(const UiRenderPacket& packet) noexcept
{
	m_uiFrameRenderer->Render(packet, m_frameGraph.get(), m_viewportRenderProducts);
	m_deviceServices.SubmitFrame(m_frameId);
	const RhiSubmissionToken graphicsToken = m_deviceServices.GetLastSubmittedToken(ERhiQueueType::Graphics);
	if (m_frameGraphExecutable)
	{
		m_referencePathTracerSession->OnFrameSubmitted(graphicsToken);
	}
	m_textureCache->RecordUploadSubmission(graphicsToken);
	m_gpuMeshCache->RecordUploadSubmission(graphicsToken);
	m_deviceServices.AdvanceFrameInFlight();
}
