#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/RhiFrameConstants.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "UI/UiRenderPacketPlayer.h"
#include "Editor/EditorTextureRegistry.h"
#include "Frame/Builders/FrameContextBuilder.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/FrameSceneResources.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Frame/RenderFrameTime.h"
#include "Frame/Lighting/ReferenceLightingInvalidation.h"
#include "Frame/Lighting/RestirLightingInvalidation.h"
#include "FrameGraph/Builder/FrameGraphFactory.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Host/RendererHost.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "Providers/ImageProviderFrameInput.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RayTracing/Scene/RayTracingPassContext.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"
#include "Scene/Preparation/RenderScenePreparation.h"
#include "Scene/RenderScene.h"
#include "Meshes/GpuMeshCache.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureCache.h"
#include "Window/Window.h"

static const auto g_framePipelineLogger = Logging::GetOrCreateLogger("Renderer.FramePipeline");

FrameUniformData FramePipeline::BuildFrameUniformData(std::uint64_t frameId, const RenderFrameTime& time) noexcept
{
	return FrameUniformData{
	    .FrameIndex = static_cast<std::uint32_t>(frameId),
	    .TotalTimeSeconds = static_cast<float>(time.UnscaledTime.count()),
	    .DeltaTimeSeconds = static_cast<float>(time.UnscaledDelta.count()),
	    .ScaledTotalTimeSeconds = static_cast<float>(time.ScaledTime.count()),
	    .ScaledDeltaTimeSeconds = static_cast<float>(time.ScaledDelta.count())};
}

FramePipeline::FramePipeline(RendererHost& rendererHost, bool enableUiRenderPackets) noexcept :
    m_rendererHost(&rendererHost),
    m_frameContextBuilder(
        rendererHost.GetRenderScene(),
        rendererHost.GetRenderScenePreparation(),
        rendererHost.GetRenderViewBuilder(),
        rendererHost.GetRenderViewPreparation()),
    m_ownsUiBackend(enableUiRenderPackets)
{
	m_windowExtent = {
	    static_cast<std::uint32_t>(rendererHost.GetWindow().GetWidth()),
	    static_cast<std::uint32_t>(rendererHost.GetWindow().GetHeight())};

	InitializeUiRendering();
	InitializeFrameStorage();
	m_displaySettings = ResolvedViewportDisplaySettings::Resolve(m_viewportRenderRequest.Exposure);
	InitializeFrameGraph();
}

void FramePipeline::InitializeUiRendering()
{
	m_uiRenderPacketPlayer = std::make_unique<UiRenderPacketPlayer>();
	m_editorTextureRegistry = std::make_unique<EditorTextureRegistry>();
	if (m_ownsUiBackend)
	{
		m_rendererHost->GetImGuiRenderer().Initialize();
	}
}

void FramePipeline::InitializeFrameStorage()
{
	const std::uint32_t maximumFramesInFlight =
	    m_rendererHost->GetRenderHardwareInterface().GetCapabilities().Presentation.MaximumFramesInFlight;
	m_frameExecutionDiagnostics.resize(maximumFramesInFlight);
	InitializeFrameContexts();

	RenderDiagnostics& backendDiagnostics = m_rendererHost->GetRenderHardwareInterface().GetDiagnostics();
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}
}

void FramePipeline::InitializeFrameContexts()
{
	m_frameContexts.clear();
	m_frameContexts.resize(m_rendererHost->GetRenderHardwareInterface().GetCapabilities().Presentation.MaximumFramesInFlight);
	for (std::unique_ptr<FrameContext>& frameContext : m_frameContexts)
	{
		frameContext = std::make_unique<FrameContext>();
	}
}

FramePipeline::~FramePipeline() noexcept
{
	if (m_ownsUiBackend)
	{
		m_uiRenderPacketPlayer->Shutdown(m_rendererHost->GetImGuiRenderer());
		m_rendererHost->GetImGuiRenderer().Shutdown();
	}
}

TextureDiagnosticsSnapshot FramePipeline::CaptureTextureDiagnostics()
{
	return m_rendererHost->CaptureTextureDiagnostics(
	    [this](std::uint64_t nativeTextureId) { return m_editorTextureRegistry->Register(nativeTextureId); });
}

void FramePipeline::RequestResize(RenderViewportExtent extent, bool minimized) noexcept
{
	m_windowExtent = extent;
	m_windowMinimized = minimized;
	m_bResizePending = true;
}

void FramePipeline::OnRender(RenderFrameSubmission submission, const RenderFrameTime& time, const UiRenderPacket& ui) noexcept
{
	m_displaySettings = ResolvedViewportDisplaySettings::Resolve(m_viewportRenderRequest.Exposure);
	if (!BeginFrame(submission))
	{
		return;
	}
	SetupFrame(time);
	RecordFrame(submission.View);
	RenderUiPacket(ui);
	SubmitFrame();
	EndFrame();
}

RenderViewportExtent FramePipeline::ResolveOutputExtent() const noexcept
{
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return m_windowExtent;
}

FrameResolutionExtents FramePipeline::ResolveFrameResolution() const noexcept
{
	const RenderViewportExtent outputExtent = ResolveOutputExtent();
	const ImageProviderPipeline imagePipeline = GetLightingMode() == LightingMode::RestirPathTraced
	    ? ImageProviderPipeline::RayReconstruction
	    : ImageProviderPipeline::PresentationUpscaling;
	return FrameResolutionExtents{
	    .Render = m_rendererHost->GetImageProviders().ResolveRenderExtent(outputExtent, imagePipeline),
	    .Output = outputExtent};
}

bool FramePipeline::ShouldOutputToBackBuffer() const noexcept
{
	return m_viewportRenderRequest.ViewportId == 0;
}

void FramePipeline::InitializeFrameGraph() noexcept
{
	InitializeFrameGraph(ResolveFrameResolution());
}

void FramePipeline::InitializeFrameGraph(FrameResolutionExtents resolution) noexcept
{
	const FramePresentationTarget presentationTarget =
	    ShouldOutputToBackBuffer() ? FramePresentationTarget::BackBuffer : FramePresentationTarget::ViewportProduct;
	const FrameGraphDependencies dependencies{
	    m_rendererHost->GetRenderHardwareInterface(),
	    m_rendererHost->GetRenderPassRuntimeCache(),
	    m_rendererHost->GetWindow(),
	    resolution.Render,
	    resolution.Output,
	    m_displaySettings.ExposureMeteringMethod,
	    presentationTarget};

	FrameGraphFactory frameGraphFactory(dependencies);
	FrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphRenderExtent = dependencies.renderExtent;
	m_frameGraphOutputExtent = dependencies.outputExtent;
	m_frameGraphPresentationTarget = dependencies.presentationTarget;
	m_frameResources = buildResult.Resources;
	m_gBufferMode = CVarGBufferMode.Get();
	m_lightingMode = GetLightingMode();
	m_exposureMeteringMethod = m_displaySettings.ExposureMeteringMethod;
	m_imageProviderFrameGraphKey = m_rendererHost->GetImageProviders().GetFrameGraphKey();
	m_frameGraph = std::move(buildResult.Graph);
	++m_graphTopologyGeneration;
}

void FramePipeline::RefreshFrameExecution(FrameResolutionExtents resolution) noexcept
{
	RetireFrameExecution();
	InitializeFrameGraph(resolution);
}

void FramePipeline::RebuildFrameExecutionAfterSwapChainDrain(FrameResolutionExtents resolution) noexcept
{
	InitializeFrameContexts();
	m_frameGraph.reset();
	InitializeFrameGraph(resolution);
}

void FramePipeline::RetireFrameExecution() noexcept
{
	if (m_frameGraph != nullptr)
	{
		m_frameExecutionRetirementQueue.Retire(m_rendererHost->GetDeviceServices(), std::move(m_frameGraph), std::move(m_frameContexts));
		InitializeFrameContexts();
	}
}

void FramePipeline::InvalidateViewHistory(RenderViewInvalidationReason reason) noexcept
{
	if (m_frameGraph != nullptr)
	{
		InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	}
	m_previousReferenceLightingHistoryInvalidationHash.reset();
	m_previousRestirLightingHistoryInvalidationHash.reset();
	m_rendererHost->GetRenderViewState().Invalidate(reason);
	m_rendererHost->GetImageProviders().ResetHistory();
}

bool FramePipeline::BeginFrame(RenderFrameSubmission& submission) noexcept
{
	PollFrameServices();
	if (!AcceptFrameSubmission(submission))
	{
		return false;
	}
	ApplyPendingResize();
	RefreshGraphForResolutionAndPresentation();
	RefreshGraphForRenderModes();
	RefreshGraphForImageProvider();
	BeginBackendFrame();
	return true;
}

void FramePipeline::PollFrameServices() noexcept
{
	PollViewportCaptures();
	m_frameExecutionRetirementQueue.Poll(m_rendererHost->GetDeviceServices());
	m_rendererHost->PollRetiredImageProviders();
	m_rendererHost->GetRenderPassRuntimeCache().PollRetiredGenerations();
	m_rendererHost->GetTextureCache().PollResidency();
	m_rendererHost->GetGpuMeshCache().PollResidency();
	m_rendererHost->GetRenderScene().PromoteResidentGpuMeshes();
}

bool FramePipeline::AcceptFrameSubmission(RenderFrameSubmission& submission) noexcept
{
	if (submission.FrameId <= m_frameId)
	{
		return false;
	}

	std::string diagnostic;
	const bool sceneReset = submission.Scene.Structural.ResetScene;
	const RenderSceneApplyStatus applyStatus =
	    m_rendererHost->GetRenderScene().Apply(submission.Scene.Structural, std::move(submission.Scene.Dynamic), diagnostic);
	if (applyStatus != RenderSceneApplyStatus::Applied)
	{
		g_framePipelineLogger->error("Render frame submission {} was rejected: {}", submission.FrameId, diagnostic);
		return false;
	}

	m_frameId = submission.FrameId;
	if (sceneReset)
	{
		m_rendererHost->GetTextureCache().UnloadSceneTextures();
		InvalidateViewHistory(RenderViewInvalidationReason::SceneGeneration);
	}
	return true;
}

void FramePipeline::ApplyPendingResize() noexcept
{
	if (m_bResizePending)
	{
		m_bResizePending = false;
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);

		if (!m_windowMinimized && m_windowExtent.IsValid())
		{
			m_rendererHost->GetDeviceServices().ResizeSwapChain();
			RebuildFrameExecutionAfterSwapChainDrain(ResolveFrameResolution());
		}
	}
}

void FramePipeline::RefreshGraphForResolutionAndPresentation() noexcept
{
	const FrameResolutionExtents frameResolution = ResolveFrameResolution();
	const FramePresentationTarget presentationTarget =
	    ShouldOutputToBackBuffer() ? FramePresentationTarget::BackBuffer : FramePresentationTarget::ViewportProduct;
	const bool resolutionChanged = frameResolution.Render.Width != m_frameGraphRenderExtent.Width
	    || frameResolution.Render.Height != m_frameGraphRenderExtent.Height
	    || frameResolution.Output.Width != m_frameGraphOutputExtent.Width
	    || frameResolution.Output.Height != m_frameGraphOutputExtent.Height;
	const bool presentationChanged = presentationTarget != m_frameGraphPresentationTarget;
	if (resolutionChanged || presentationChanged)
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(frameResolution);
	}
}

void FramePipeline::RefreshGraphForRenderModes() noexcept
{
	const GBufferMode gBufferMode = CVarGBufferMode.Get();
	if (gBufferMode != m_gBufferMode)
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(ResolveFrameResolution());
	}

	const LightingMode lightingMode = GetLightingMode();
	if (lightingMode != m_lightingMode)
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(ResolveFrameResolution());
	}

	if (m_displaySettings.ExposureMeteringMethod != m_exposureMeteringMethod)
	{
		InvalidateViewHistory(RenderViewInvalidationReason::GraphTopology);
		RefreshFrameExecution(ResolveFrameResolution());
	}
}

void FramePipeline::RefreshGraphForImageProvider() noexcept
{
	const ImageProviderGraphKey imageProviderFrameGraphKey = m_rendererHost->GetImageProviders().GetFrameGraphKey();
	if (imageProviderFrameGraphKey != m_imageProviderFrameGraphKey)
	{
		m_rendererHost->RefreshImageProviders();
		RefreshFrameExecution(ResolveFrameResolution());
		m_imageProviderFrameGraphKey = imageProviderFrameGraphKey;
	}
}

void FramePipeline::BeginBackendFrame() noexcept
{
	RenderDeviceServices& deviceServices = m_rendererHost->GetDeviceServices();
	deviceServices.BeginFrame(m_frameId);
	if (m_ownsUiBackend)
	{
		m_rendererHost->GetImGuiRenderer().BeginFrame();
	}

	m_rendererHost->TickDiagnostics(m_rendererHost->GetRenderHardwareInterface().GetCurrentFrameIndex());
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
}

void FramePipeline::SetupFrame(const RenderFrameTime& time) noexcept
{
	RefreshViewportRenderProducts();

	RenderDeviceServices& deviceServices = m_rendererHost->GetDeviceServices();
	RenderCommandList& graphicsCommandList = deviceServices.GetCurrentGraphicsCommandList();
	m_rendererHost->GetGpuMeshCache().UploadReadyMeshes(graphicsCommandList);
	UploadPendingSceneTextures(deviceServices, graphicsCommandList);

	m_frameUniform = BuildFrameUniformData(m_frameId, time);
}

void FramePipeline::UploadPendingSceneTextures(RenderDeviceServices& deviceServices, RenderCommandList& graphicsCommandList)
{
	TextureCache& textureCache = m_rendererHost->GetTextureCache();
	const bool useCopyQueue = textureCache.HasPendingSceneTextureUploads()
	    && m_rendererHost->GetRenderHardwareInterface().GetCapabilities().Queues.SupportsIndependent(ERhiQueueType::Copy);

	RhiCommandRecordingLease uploadLease;
	RenderCommandList* uploadCommandList = &graphicsCommandList;
	if (useCopyQueue)
	{
		uploadLease = deviceServices.AcquireCommandRecordingLease(ERhiQueueType::Copy);
		uploadCommandList = &uploadLease.GetCommandList();
	}

	const std::vector<RhiResourceHandle> uploadedResources =
	    textureCache.LoadSceneTextures(m_rendererHost->GetRenderScene().GetTextures(), *uploadCommandList);
	if (useCopyQueue)
	{
		const RhiSubmissionToken uploadToken = deviceServices.SubmitCommandRecordingLease(std::move(uploadLease));
		textureCache.RecordUploadSubmission(uploadToken);
		deviceServices.QueueWait(ERhiQueueType::Graphics, uploadToken);
		for (const RhiResourceHandle resource : uploadedResources)
		{
			graphicsCommandList.TransitionResource(resource, ResourceState::Common, ResourceState::ShaderResource);
		}
	}
}

void FramePipeline::RefreshViewportRenderProducts() noexcept
{
	const FrameResolutionExtents resolution = m_frameGraphOutputExtent.IsValid() && m_frameGraphRenderExtent.IsValid()
	    ? FrameResolutionExtents{.Render = m_frameGraphRenderExtent, .Output = m_frameGraphOutputExtent}
	    : ResolveFrameResolution();

	m_viewportRenderProducts.Clear();
	m_viewportRenderProducts.SetGeneration(m_viewportRenderRequest.Generation);
	m_viewportRenderProducts.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{
	        .Handle = ToRenderProductHandle(m_frameResources.ViewportProducts.FinalSceneColor),
	        .Extent = resolution.Output,
	        .Format = RenderProductFormat::ColorLdr});

	if (m_frameResources.ViewportProducts.SceneDepth.IsValid()
	    && HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{
		        .Handle = ToRenderProductHandle(m_frameResources.ViewportProducts.SceneDepth),
		        .Extent = resolution.Render,
		        .Format = RenderProductFormat::Float});
	}

	if (m_frameResources.ViewportProducts.Normals.IsValid()
	    && HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::Normals))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::Normals,
		    RenderProduct{
		        .Handle = ToRenderProductHandle(m_frameResources.ViewportProducts.Normals),
		        .Extent = resolution.Render,
		        .Format = RenderProductFormat::ColorHdr});
	}
}

void FramePipeline::RenderUiPacket(const UiRenderPacket& packet) noexcept
{
	switch (packet.PresentationMode)
	{
		case UiPresentationMode::HostOverlay:
			RenderHostOverlayUi(packet);
			break;
		case UiPresentationMode::EditorViewport:
			RenderEditorViewportUi(packet);
			break;
		case UiPresentationMode::None:
		default:
			break;
	}
}

void FramePipeline::RenderEditorViewportUi(const UiRenderPacket& packet) noexcept
{
	if (!BeginViewportEditorTexturePresentation(RenderOutputFlags::SceneColor))
	{
		m_editorTextureRegistry->RetireViewportTexture();
		return;
	}

	if (!packet.HasDrawData() || packet.ViewportGeneration != m_viewportRenderProducts.GetGeneration())
	{
		EndViewportEditorTexturePresentation(RenderOutputFlags::SceneColor);
		return;
	}

	constexpr float clearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	RhiPresentationService& hostPresentation = m_rendererHost->GetRenderHardwareInterface().GetPresentationService();
	hostPresentation.BeginPresentRenderPass(clearColor);
	PlayUiPacket(packet);
	hostPresentation.EndPresentRenderPass();
	EndViewportEditorTexturePresentation(RenderOutputFlags::SceneColor);
}

void FramePipeline::RenderHostOverlayUi(const UiRenderPacket& packet) noexcept
{
	if (!packet.HasDrawData())
	{
		return;
	}

	RhiPresentationService& presentation = m_rendererHost->GetRenderHardwareInterface().GetPresentationService();
	presentation.BeginPresentOverlayPass();
	PlayUiPacket(packet);
	presentation.EndPresentRenderPass();
}

void FramePipeline::PlayUiPacket(const UiRenderPacket& packet) noexcept
{
	RhiImGuiRenderer& imguiRenderer = m_rendererHost->GetImGuiRenderer();
	m_uiRenderPacketPlayer->Render(packet, *m_editorTextureRegistry, imguiRenderer);
}

void FramePipeline::RecordFrame(const RenderViewInput& viewInput) noexcept
{
	RenderScene& renderScene = m_rendererHost->GetRenderScene();
	FrameContext& frame = PrepareFrameContext(viewInput);
	UpdateLightingHistory(frame);
	SetupImageProviderFrame(frame);
	if (!renderScene.IsRayTracingAvailable())
	{
		Diagnostics::Fatal(g_framePipelineLogger, __FILE__, __LINE__, "Ray-tracing scene capability is unavailable.");
	}
	const RenderRayTracingFrameBindings rayTracingBindings = renderScene.PrepareRayTracingFrame(frame.preparedScene);
	BindRayTracingScene(frame, rayTracingBindings);
	BindSkyTexture(frame);
	BindRenderSceneGpuResources(*m_frameGraph, m_frameResources.External.Scene, *frame.preparedScene.gpuBindings);
	ExecuteFrameGraph(frame);
}

FrameContext& FramePipeline::PrepareFrameContext(const RenderViewInput& viewInput)
{
	const std::uint32_t frameIndex = m_rendererHost->GetRenderHardwareInterface().GetCurrentFrameIndex();
	std::unique_ptr<FrameContext>& frameSlot = m_frameContexts[frameIndex];
	m_frameContextBuilder.Build(
	    *frameSlot,
	    FrameContextBuildRequest{
	        .ViewState = m_rendererHost->GetRenderViewState(),
	        .ViewInput = viewInput,
	        .ViewportRequest = m_viewportRenderRequest,
	        .FrameId = m_frameId,
	        .ShaderGeneration = m_rendererHost->GetShaderPackageGeneration(),
	        .ImageProviderGeneration = m_rendererHost->GetImageProviderGeneration(),
	        .GraphTopologyGeneration = m_graphTopologyGeneration,
	        .FrameIndex = frameIndex,
	        .RenderExtent = m_frameGraphRenderExtent,
	        .OutputExtent = m_frameGraphOutputExtent,
	        .ViewMode = CVarRenderViewMode.Get(),
	    });
	return *frameSlot;
}

void FramePipeline::UpdateLightingHistory(FrameContext& frame)
{
	if (GetLightingMode() == LightingMode::RestirPathTraced)
	{
		const std::uint64_t invalidationHash = BuildRestirLightingHistoryInvalidationHash(frame);
		const bool invalidateHistory =
		    !m_previousRestirLightingHistoryInvalidationHash || invalidationHash != *m_previousRestirLightingHistoryInvalidationHash;
		if (invalidateHistory)
		{
			InvalidateRestirLightingHistory(*m_frameGraph, m_frameResources.History);
			m_rendererHost->GetImageProviders().ResetHistory();
		}
		m_previousRestirLightingHistoryInvalidationHash = invalidationHash;
	}
	else if (GetLightingMode() == LightingMode::ReferencePathTraced)
	{
		const std::uint64_t invalidationHash = BuildReferenceLightingHistoryInvalidationHash(frame);
		const bool invalidateHistory =
		    !m_previousReferenceLightingHistoryInvalidationHash || invalidationHash != *m_previousReferenceLightingHistoryInvalidationHash;
		if (invalidateHistory)
		{
			m_frameGraph->InvalidateTextureHistory(m_frameResources.History.ReferenceLighting);
		}
		m_previousReferenceLightingHistoryInvalidationHash = invalidationHash;
	}

	if (frame.view.temporalUniform.HistoryValid == 0u)
	{
		InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	}
}

void FramePipeline::SetupImageProviderFrame(const FrameContext& frame)
{
	m_rendererHost->GetImageProviders().SetupFrame(
	    ImageProviderFrameInput{
	        .RenderExtent = frame.view.renderExtent,
	        .OutputExtent = frame.view.outputExtent,
	        .FrameId = m_frameId,
	        .ProviderGeneration = m_rendererHost->GetImageProviderGeneration(),
	        .Camera = frame.view.cameraUniform,
	        .Temporal = frame.view.temporalUniform,
	        .ResetHistory = frame.view.temporalUniform.HistoryValid == 0u});
}

void FramePipeline::BindRayTracingScene(const FrameContext& frame, const RenderRayTracingFrameBindings& rayTracingBindings)
{
	if (m_frameGraph == nullptr || !m_frameResources.SceneTlas.IsValid())
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene producer or persistent SceneTlas resource is unavailable.");
	}

	if (!rayTracingBindings.HasBoundTlas() || rayTracingBindings.TlasShaderAccessMode != RayTracingSceneTlasShaderAccessMode::Descriptor)
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene producer failed to provide the descriptor-access SceneTlas consumed by frame shaders.");
	}
	if (!frame.preparedScene.materialTextureTable || !frame.preparedScene.gpuBindings->RayTracing.HasCompleteBuffers()
	    || !frame.preparedScene.gpuBindings->Geometry.HasMeshInstanceBuffers())
	{
		Diagnostics::Fatal(g_framePipelineLogger, __FILE__, __LINE__, "Ray-tracing frame shader resources are incomplete.");
	}
	if (rayTracingBindings.HasTraceableInstances()
	    && (frame.preparedScene.gpuBindings->RayTracing.InstanceCount == 0u
	        || frame.preparedScene.gpuBindings->RayTracing.MaterialCount == 0u))
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Traceable SceneTlas instances have no matching hit-instance or material records.");
	}

	m_frameGraph->BindPersistentAccelerationStructure(
	    m_frameResources.SceneTlas,
	    rayTracingBindings.TlasResource,
	    rayTracingBindings.TlasGpuAddress);
}

void FramePipeline::BindSkyTexture(const FrameContext& frame)
{
	if (!frame.preparedScene.sky.HasTexture())
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "TextureCache did not provide a valid sky texture before frame recording.");
	}

	const RendererTexture& skyTexture = *frame.preparedScene.sky.texture;
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.External.Sky,
	    skyTexture.Resource,
	    skyTexture.ShaderResourceView,
	    FrameGraphTextureDesc::CreateColor("Sky", skyTexture.Width, skyTexture.Height, skyTexture.Format),
	    ResourceState::ShaderResource);
}

void FramePipeline::ExecuteFrameGraph(FrameContext& frame)
{
	RenderHardwareInterface& renderHardwareInterface = m_rendererHost->GetRenderHardwareInterface();
	const RayTracedShadowSettings shadowSettings{
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get()};
	const bool rayTracedShadowsEnabled = CVarRayTracedShadowsEnabled.Get();

	m_frameGraph->Setup(frame);
	const FrameGraphPlan& compiledPlan = m_frameGraph->Compile();

	const RayTracingPassContext rayTracingPassContext{
	    .Scene = &m_rendererHost->GetRenderScene(),
	    .CapabilityReport = &m_rendererHost->GetRenderScene().GetRayTracingCapabilities(),
	    .ShadowSettings = &shadowSettings,
	    .ShadowsEnabled = rayTracedShadowsEnabled};
	const ImageProviderPassContext imageProviderPassContext = m_rendererHost->GetImageProviders().BuildPassContext();

	const PassRuntimeContext passRuntimeContext{
	    .HardwareInterface = renderHardwareInterface,
	    .PassRuntimes = m_rendererHost->GetRenderPassRuntimeCache(),
	    .Frame = m_frameUniform,
	    .DisplaySettings = m_displaySettings,
	    .History = ResolveFrameHistoryValidity(*m_frameGraph, m_frameResources.History),
	    .Meshes = &m_rendererHost->GetGpuMeshCache(),
	    .Textures = &m_rendererHost->GetTextureCache(),
	    .RayTracing = &rayTracingPassContext,
	    .ImageProviders = &imageProviderPassContext};
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	m_frameGraph->Execute(
	    compiledPlan,
	    m_rendererHost->GetDeviceServices(),
	    frame,
	    passRuntimeContext,
	    frameDiagnostics,
	    m_rendererHost->GetTaskExecutor());
}

void FramePipeline::SubmitFrame() noexcept
{
	RenderDeviceServices& deviceServices = m_rendererHost->GetDeviceServices();
	deviceServices.SubmitFrame(m_frameId);
	const RhiSubmissionToken graphicsToken = deviceServices.GetLastSubmittedToken(ERhiQueueType::Graphics);
	m_rendererHost->GetTextureCache().RecordUploadSubmission(graphicsToken);
	m_rendererHost->GetGpuMeshCache().RecordUploadSubmission(graphicsToken);
}

void FramePipeline::EndFrame() noexcept
{
	m_rendererHost->GetDeviceServices().AdvanceFrameInFlight();
}
