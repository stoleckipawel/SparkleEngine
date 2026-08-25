#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "UI/UiFrameRenderer.h"
#include "Frame/RenderFrame.h"
#include "Frame/Graph/ExecuteRenderFrameGraph.h"
#include "Frame/Graph/RenderProductGraphHandle.h"
#include "Frame/RenderFrameTime.h"
#include "Frame/Graph/RenderFrameGraphFactory.h"
#include "FrameGraph/FrameGraph.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "Providers/ImageProviderFrameInput.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Preparation/RenderScenePreparation.h"
#include "Scene/RenderScene.h"
#include "Meshes/GpuMeshCache.h"
#include "Textures/TextureCache.h"
#include "View/RenderViewBuilder.h"
#include "View/RenderViewPreparation.h"
#include "Viewport/ViewportCaptureService.h"
#include "Viewport/ViewportRenderProductPublication.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(
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
    bool enableUiRenderPackets) noexcept :
    m_window(window),
    m_deviceServices(deviceServices),
    m_renderPassRuntimeCache(renderPassRuntimeCache),
    m_memoryMonitor(memoryMonitor),
    m_gpuMeshCache(gpuMeshCache),
    m_textureCache(textureCache),
    m_renderScenePreparation(renderScenePreparation),
    m_renderViewBuilder(renderViewBuilder),
    m_renderViewPreparation(renderViewPreparation),
    m_renderViewState(renderViewState),
    m_renderScene(renderScene),
    m_imageProviders(imageProviders),
    m_taskExecutor(taskExecutor),
    m_uiFrameRenderer(std::make_unique<UiFrameRenderer>(deviceServices, enableUiRenderPackets)),
    m_viewportCaptureService(std::make_unique<ViewportCaptureService>(deviceServices))
{
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

bool FramePipeline::BeginViewportCapture(ViewportCaptureId id, const ViewportCaptureRequest& request) noexcept
{
	return m_viewportCaptureService->BeginCapture(
	    id,
	    request,
	    m_viewportRenderProducts,
	    m_frameGraph.get(),
	    m_frameId,
	    m_renderScene.GetSceneGeneration(),
	    m_imageProviders.GetGeneration());
}

std::vector<ViewportCaptureReadback> FramePipeline::TakeCompletedViewportCaptures()
{
	return m_viewportCaptureService->TakeCompletedCaptures();
}

TextureDiagnosticsSnapshot FramePipeline::CaptureTextureDiagnostics()
{
	return m_textureCache.CaptureDiagnosticsSnapshot(
	    [this](std::uint64_t nativeTextureId) { return m_uiFrameRenderer->RegisterEditorTexture(nativeTextureId); });
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
	const RenderRayTracingFrameBindings rayTracingBindings = PrepareFrame(submission.View, time);
	ExecuteFrame(rayTracingBindings);
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
	m_imageProviders.PollRetiredGenerations();
	m_renderPassRuntimeCache.PollRetiredGenerations();
	m_textureCache.PollResidency();
	m_gpuMeshCache.PollResidency();
	m_renderScene.PromoteResidentGpuMeshes();
}

bool FramePipeline::AcceptFrameSubmission(RenderFrameSubmission& submission) noexcept
{
	if (submission.FrameId <= m_frameId)
	{
		return false;
	}

	const bool sceneReset = submission.Scene.Structural.ResetScene;
	if (!m_renderScene.Apply(submission.Scene.Structural, std::move(submission.Scene.Dynamic)))
	{
		return false;
	}

	m_frameId = submission.FrameId;
	if (sceneReset)
	{
		m_textureCache.UnloadSceneTextures();
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

RenderRayTracingFrameBindings FramePipeline::PrepareFrame(const RenderViewInput& viewInput, const RenderFrameTime& time)
{
	const RenderFrameGraphSettings viewportSettings =
	    m_frameGraphSettings.OutputExtent.IsValid() && m_frameGraphSettings.RenderExtent.IsValid() ? m_frameGraphSettings
	                                                                                               : ResolveFrameGraphSettings();
	PublishViewportRenderProducts(
	    m_viewportRenderProducts,
	    m_viewportRenderRequest,
	    m_frameResources.ViewportProducts,
	    viewportSettings.RenderExtent,
	    viewportSettings.OutputExtent);

	RenderCommandList& graphicsCommandList = m_deviceServices.GetCurrentGraphicsCommandList();
	m_gpuMeshCache.UploadReadyMeshes(graphicsCommandList);
	m_textureCache.UpdateSceneTextures(m_renderScene.GetTextures(), m_deviceServices);

	const RenderFrame& frame = PrepareRenderFrame(viewInput, time);
	UpdateFrameHistory(
	    *m_frameGraph,
	    m_frameResources.History,
	    frame.PreparedScene,
	    frame.View,
	    m_renderViewState,
	    m_imageProviders);
	SetupImageProviderFrame(frame);
	return m_renderScene.PrepareRayTracingFrame(frame.PreparedScene, frame.View.rayTracingPlan);
}

void FramePipeline::ExecuteFrame(const RenderRayTracingFrameBindings& rayTracingBindings)
{
	const std::uint32_t frameIndex = m_deviceServices.GetRenderHardwareInterface().GetCurrentFrameIndex();
	const RenderFrame& frame = *m_renderFrames[frameIndex];
	RenderFrameGraphExecution::Execute(
	    *m_frameGraph,
	    m_frameResources,
	    frame.Identity,
	    frame.Time,
	    frame.PreparedScene,
	    frame.View,
	    rayTracingBindings,
	    m_deviceServices,
	    GetCurrentFrameDiagnostics(),
	    m_taskExecutor);
}

RenderFrame& FramePipeline::PrepareRenderFrame(const RenderViewInput& viewInput, const RenderFrameTime& time)
{
	const std::uint32_t frameIndex = m_deviceServices.GetRenderHardwareInterface().GetCurrentFrameIndex();
	std::unique_ptr<RenderFrame>& frameSlot = m_renderFrames[frameIndex];
	RenderFrame& frame = *frameSlot;
	RenderScene& scene = m_renderScene;
	frame.Identity = RenderFrameIdentity{
	    .FrameId = m_frameId,
	    .ShaderGeneration = m_renderPassRuntimeCache.GetShaderGeneration(),
	    .ImageProviderGeneration = m_imageProviders.GetGeneration()};
	frame.Time = time;
	frame.FrameInFlightIndex = frameIndex;

	m_renderScenePreparation.Execute(scene, frame.PreparedScene);
	m_renderViewBuilder.Build(
	    frame.View,
	    m_renderViewState,
	    RenderViewBuildRequest{
	        .Input = viewInput,
	        .ViewportRequest = m_viewportRenderRequest,
	        .RenderExtent = m_frameGraphSettings.RenderExtent,
	        .OutputExtent = m_frameGraphSettings.OutputExtent,
	        .FrameId = frame.Identity.FrameId,
	        .SceneGeneration = scene.GetSceneGeneration(),
	        .ShaderGeneration = frame.Identity.ShaderGeneration,
	        .ImageProviderGeneration = frame.Identity.ImageProviderGeneration,
	        .GraphTopologyGeneration = m_graphTopologyGeneration});
	m_renderViewPreparation.Prepare(frame.PreparedScene, frame.View, m_renderViewState);
	frame.PreparedScene.gpuBindings = &scene.UpdateGpuScene(frame.PreparedScene, frame.View, frame.FrameInFlightIndex);
	return *frameSlot;
}

void FramePipeline::SetupImageProviderFrame(const RenderFrame& frame)
{
	m_imageProviders.SetupFrame(
	    ImageProviderFrameInput{
	        .RenderExtent = frame.View.renderExtent,
	        .OutputExtent = frame.View.outputExtent,
	        .FrameId = frame.Identity.FrameId,
	        .ProviderGeneration = frame.Identity.ImageProviderGeneration,
	        .Camera = frame.View.cameraUniform,
	        .Temporal = frame.View.temporalUniform,
	        .ResetHistory = frame.View.temporalUniform.HistoryValid == 0u});
}

void FramePipeline::SubmitAndPresent(const UiRenderPacket& packet) noexcept
{
	m_uiFrameRenderer->Render(packet, m_frameGraph.get(), m_viewportRenderProducts);
	m_deviceServices.SubmitFrame(m_frameId);
	const RhiSubmissionToken graphicsToken = m_deviceServices.GetLastSubmittedToken(ERhiQueueType::Graphics);
	m_textureCache.RecordUploadSubmission(graphicsToken);
	m_gpuMeshCache.RecordUploadSubmission(graphicsToken);
	m_deviceServices.AdvanceFrameInFlight();
}
