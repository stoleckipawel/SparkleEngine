#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Camera/RenderCamera.h"
#include "Commands/RenderCommandContext.h"
#include "Frame/RhiFrameConstants.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "UI/UiRenderPacketPlayer.h"
#include "Editor/EditorTextureRegistry.h"
#include "Frame/Builders/FrameContextBuilder.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Frame/Lighting/ReferenceLightingInvalidation.h"
#include "Frame/Lighting/RestirLightingInvalidation.h"
#include "FrameGraph/Builder/FrameGraphFactory.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Host/RendererHost.h"
#include "Pipeline/RenderPassRuntimeCache.h"
#include "Providers/RendererImageProviderStack.h"
#include "Providers/ImageProviderFrameContext.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RayTracing/Scene/RayTracingPassContext.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"
#include "SceneData/Preparation/RenderPreparationGraph.h"
#include "SceneData/Caching/MaterialCache.h"
#include "SceneData/GpuScene/PersistentRenderGpuScene.h"
#include "SceneData/Input/RenderInputConsumer.h"
#include "SceneData/RenderWorld.h"
#include "Meshes/GpuMeshCache.h"
#include "SceneData/RenderSceneGpuData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureCache.h"
#include "Time/Timer.h"
#include "Window/Window.h"

static const auto g_framePipelineLogger = Logging::GetOrCreateLogger("Renderer.FramePipeline");

FramePipeline::FramePipeline(RendererHost& rendererHost, bool enableUiRenderPackets) noexcept :
    m_rendererHost(&rendererHost), m_ownsUiBackend(enableUiRenderPackets)
{
	m_windowExtent = {
	    static_cast<std::uint32_t>(rendererHost.GetWindow().GetWidth()),
	    static_cast<std::uint32_t>(rendererHost.GetWindow().GetHeight())};

	InitializeSceneData();
	InitializeUiRendering();
	InitializeFrameStorage();
	InitializeFrameGraph();
}

void FramePipeline::InitializeSceneData()
{
	m_renderInputConsumer = std::make_unique<RenderInputConsumer>(m_rendererHost->GetRenderWorld());

	m_gpuScene = std::make_unique<PersistentRenderGpuScene>(
	    m_rendererHost->GetRenderHardwareInterface().GetResourceService(),
	    m_rendererHost->GetGpuMeshCache());
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

void FramePipeline::SubmitRenderInput(RenderInputFrame input) noexcept
{
	FinalizeRenderInputMetadata(input);
	(void) m_renderInputConsumer->Submit(std::move(input));
}

void FramePipeline::FinalizeRenderInputMetadata(RenderInputFrame& input) const noexcept
{
	const FrameResolutionExtents resolution = ResolveFrameResolution();
	input.Dynamic.Metadata.RenderWidth = resolution.Render.Width;
	input.Dynamic.Metadata.RenderHeight = resolution.Render.Height;
	input.Dynamic.Metadata.OutputWidth = resolution.Output.Width;
	input.Dynamic.Metadata.OutputHeight = resolution.Output.Height;
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
	    [this](std::uint64_t nativeTextureId)
	    {
		    return m_editorTextureRegistry->Register(nativeTextureId);
	    });
}

void FramePipeline::RequestResize(RenderViewportExtent extent, bool minimized) noexcept
{
	m_windowExtent = extent;
	m_windowMinimized = minimized;
	m_bResizePending = true;
}

void FramePipeline::OnRender(const TimeInfo& timing, const UiRenderPacket& ui) noexcept
{
	BeginFrame();
	SetupFrame(timing);
	RecordFrame();
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
	const FrameGraphDependencies dependencies{
	    m_rendererHost->GetRenderHardwareInterface(),
	    m_rendererHost->GetRenderPassRuntimeCache(),
	    m_rendererHost->GetWindow(),
	    resolution.Render,
	    resolution.Output,
	    ShouldOutputToBackBuffer()};

	FrameGraphFactory frameGraphFactory(dependencies);
	FrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphRenderExtent = dependencies.renderExtent;
	m_frameGraphOutputExtent = dependencies.outputExtent;
	m_frameGraphPresentsToBackBuffer = dependencies.presentSceneToBackBuffer;
	m_frameResources = buildResult.Resources;
	m_gBufferMode = CVarGBufferMode.Get();
	m_lightingMode = GetLightingMode();
	m_imageProviderFrameGraphKey = m_rendererHost->GetImageProviders().GetFrameGraphKey();
	m_frameGraph = std::move(buildResult.Graph);
}

void FramePipeline::RefreshFrameExecution() noexcept
{
	RefreshFrameExecution(ResolveFrameResolution());
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

void FramePipeline::ResetTemporalState(std::string_view reason) noexcept
{
	InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	m_previousReferenceLightingHistoryInvalidationHash.reset();
	m_previousRestirLightingHistoryInvalidationHash.reset();
	m_rendererHost->GetTemporalDataBuilder().ResetHistory(reason);
	m_rendererHost->GetImageProviders().ResetHistory();
}

void FramePipeline::BeginFrame() noexcept
{
	PollFrameServices();
	ConsumeRenderInput();
	ApplyPendingResize();
	RefreshGraphForResolutionAndPresentation();
	RefreshGraphForRenderModes();
	RefreshGraphForImageProvider();
	BeginBackendFrame();
}

void FramePipeline::PollFrameServices() noexcept
{
	PollViewportCaptures();
	m_frameExecutionRetirementQueue.Poll(m_rendererHost->GetDeviceServices());
	m_rendererHost->PollRetiredImageProviders();
	m_rendererHost->GetRenderPassRuntimeCache().PollRetiredGenerations();
	m_rendererHost->GetTextureCache().PollResidency();
	m_rendererHost->GetGpuMeshCache().PollResidency();
	m_rendererHost->GetRenderWorld().PromoteResidentGpuMeshes();
}

void FramePipeline::ConsumeRenderInput() noexcept
{
	const RenderInputConsumeResult inputResult = m_renderInputConsumer->ConsumePending();
	if (inputResult.SceneReset)
	{
		m_gpuScene->Reset();
		m_rendererHost->GetTextureCache().UnloadSceneTextures();
	}
}

void FramePipeline::ApplyPendingResize() noexcept
{
	if (m_bResizePending)
	{
		m_bResizePending = false;
		ResetTemporalState("Window resize");

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
	const bool presentsToBackBuffer = ShouldOutputToBackBuffer();
	const bool resolutionChanged = frameResolution.Render.Width != m_frameGraphRenderExtent.Width ||
	                               frameResolution.Render.Height != m_frameGraphRenderExtent.Height ||
	                               frameResolution.Output.Width != m_frameGraphOutputExtent.Width ||
	                               frameResolution.Output.Height != m_frameGraphOutputExtent.Height;
	const bool presentationChanged = presentsToBackBuffer != m_frameGraphPresentsToBackBuffer;
	if (resolutionChanged || presentationChanged)
	{
		ResetTemporalState(presentationChanged ? "Frame presentation mode changed" : "Frame resolution changed");
		RefreshFrameExecution(frameResolution);
	}
}

void FramePipeline::RefreshGraphForRenderModes() noexcept
{
	const GBufferMode gBufferMode = CVarGBufferMode.Get();
	if (gBufferMode != m_gBufferMode)
	{
		ResetTemporalState("GBuffer mode changed");
		RefreshFrameExecution(ResolveFrameResolution());
	}

	const LightingMode lightingMode = GetLightingMode();
	if (lightingMode != m_lightingMode)
	{
		ResetTemporalState("Lighting mode changed");
		RefreshFrameExecution(ResolveFrameResolution());
	}
}

void FramePipeline::RefreshGraphForImageProvider() noexcept
{
	const ImageProviderGraphKey imageProviderFrameGraphKey = m_rendererHost->GetImageProviders().GetFrameGraphKey();
	if (imageProviderFrameGraphKey != m_imageProviderFrameGraphKey)
	{
		m_rendererHost->RefreshImageProviders();
		ResetTemporalState("Image provider graph mode changed");
		RefreshFrameExecution(ResolveFrameResolution());
		m_imageProviderFrameGraphKey = imageProviderFrameGraphKey;
	}
}

void FramePipeline::BeginBackendFrame() noexcept
{
	RenderDeviceServices& deviceServices = m_rendererHost->GetDeviceServices();
	deviceServices.BeginFrame(m_renderInputConsumer->GetDynamicData().Metadata.FrameId);
	if (m_ownsUiBackend)
	{
		m_rendererHost->GetImGuiRenderer().BeginFrame();
	}

	m_rendererHost->TickDiagnostics(m_rendererHost->GetRenderHardwareInterface().GetCurrentFrameIndex());
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
}

void FramePipeline::SetupFrame(const TimeInfo& timing) noexcept
{
	RefreshViewportRenderProducts();

	RenderDeviceServices& deviceServices = m_rendererHost->GetDeviceServices();
	RenderCommandList& graphicsCommandList = deviceServices.GetCurrentGraphicsCommandList();
	m_rendererHost->GetGpuMeshCache().UploadReadyMeshes(graphicsCommandList);
	UploadPendingSceneTextures(deviceServices, graphicsCommandList);

	m_rendererHost->GetRenderCamera().Update(m_renderInputConsumer->GetDynamicData().Camera);

	const RenderViewportExtent renderExtent =
	    m_frameGraphRenderExtent.IsValid() ? m_frameGraphRenderExtent : ResolveFrameResolution().Render;
	m_perFrameData = m_perFrameDataBuilder.Build(timing, CVarRenderViewMode.Get(), renderExtent);
}

void FramePipeline::UploadPendingSceneTextures(RenderDeviceServices& deviceServices, RenderCommandList& graphicsCommandList)
{
	TextureCache& textureCache = m_rendererHost->GetTextureCache();
	const bool useCopyQueue =
	    textureCache.HasPendingSceneTextureUploads() &&
	    m_rendererHost->GetRenderHardwareInterface().GetCapabilities().Queues.SupportsIndependent(ERhiQueueType::Copy);

	RhiCommandRecordingLease uploadLease;
	RenderCommandList* uploadCommandList = &graphicsCommandList;
	if (useCopyQueue)
	{
		uploadLease = deviceServices.AcquireCommandRecordingLease(ERhiQueueType::Copy);
		uploadCommandList = &uploadLease.GetCommandList();
	}

	const std::vector<RhiResourceHandle> uploadedResources =
	    textureCache.LoadSceneTextures(m_rendererHost->GetRenderWorld().GetTextures(), *uploadCommandList);
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
	const FrameResolutionExtents resolution =
	    m_frameGraphOutputExtent.IsValid() && m_frameGraphRenderExtent.IsValid()
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

	if (m_frameResources.ViewportProducts.SceneDepth.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{
		        .Handle = ToRenderProductHandle(m_frameResources.ViewportProducts.SceneDepth),
		        .Extent = resolution.Render,
		        .Format = RenderProductFormat::Float});
	}

	if (m_frameResources.ViewportProducts.Normals.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::Normals))
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

void FramePipeline::RecordFrame() noexcept
{
	const RenderFrameDynamicData& dynamic = m_renderInputConsumer->GetDynamicData();
	RenderRayTracingScene* activeRayTracingScene = m_rendererHost->GetRenderRayTracingScene();

	ApplyRenderInputHistoryReset(dynamic);
	FrameContext& frame = PrepareFrameContext(dynamic, activeRayTracingScene);
	UpdateLightingHistory(frame);
	SetupImageProviderFrame(frame, dynamic);
	BindRayTracingScene(frame, activeRayTracingScene);
	BindSkyTexture(frame);
	BindRenderSceneGpuResources(*m_frameGraph, m_frameResources.External.Scene, *frame.sceneGpuData);
	ExecuteFrameGraph(frame, activeRayTracingScene);
}

void FramePipeline::ApplyRenderInputHistoryReset(const RenderFrameDynamicData& dynamic) noexcept
{
	if (dynamic.Metadata.ResetHistory || m_rendererHost->GetRenderWorld().ConsumeHistoryReset())
	{
		ResetTemporalState(dynamic.Metadata.CameraCut ? "Render input camera cut" : "Render input generation reset");
	}
}

FrameContext& FramePipeline::PrepareFrameContext(const RenderFrameDynamicData& dynamic, RenderRayTracingScene* activeRayTracingScene)
{
	const std::uint32_t frameIndex = m_rendererHost->GetRenderHardwareInterface().GetCurrentFrameIndex();
	std::unique_ptr<FrameContext>& frameSlot = m_frameContexts[frameIndex];
	FrameContextBuilder::Build(
	    *frameSlot,
	    m_rendererHost->GetRenderWorld(),
	    dynamic,
	    *m_gpuScene,
	    frameIndex,
	    m_rendererHost->GetRenderCamera(),
	    m_frameGraphRenderExtent,
	    m_rendererHost->GetRenderPreparationGraph(),
	    activeRayTracingScene,
	    m_rendererHost->GetPerViewDataBuilder(),
	    m_rendererHost->GetTemporalDataBuilder());
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

	if (frame.mainView.perTemporalData.HistoryValid == 0u)
	{
		InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	}
}

void FramePipeline::SetupImageProviderFrame(const FrameContext& frame, const RenderFrameDynamicData& dynamic)
{
	m_rendererHost->GetImageProviders().SetupFrame(
	    ImageProviderFrameContext{
	        .RenderExtent = m_frameGraphRenderExtent,
	        .OutputExtent = m_frameGraphOutputExtent,
	        .FrameId = dynamic.Metadata.FrameId,
	        .ProviderGeneration = dynamic.Metadata.ProviderGeneration,
	        .Camera = frame.mainView.perViewData.Camera,
	        .TemporalData = frame.mainView.perTemporalData,
	        .TemporalState = frame.mainView.temporalState,
	        .ResetHistory = frame.mainView.perTemporalData.HistoryValid == 0u});
}

void FramePipeline::BindRayTracingScene(FrameContext& frame, RenderRayTracingScene* activeRayTracingScene)
{
	if (m_frameGraph == nullptr || !m_frameResources.SceneTlas.IsValid() || activeRayTracingScene == nullptr ||
	    !activeRayTracingScene->IsAvailable())
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene producer or persistent SceneTlas resource is unavailable.");
	}

	frame.rayTracingScene = activeRayTracingScene->Prepare(frame.sceneData);
	if (!frame.rayTracingScene.HasBoundTlas() ||
	    frame.rayTracingScene.TlasShaderAccessMode != RayTracingSceneTlasShaderAccessMode::Descriptor)
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing scene producer failed to provide the descriptor-access SceneTlas consumed by frame shaders.");
	}
	if (!frame.sceneData.materialTextureTable || !frame.sceneGpuData->RayTracing.HasCompleteBuffers() ||
	    !frame.sceneGpuData->Geometry.HasMeshInstanceBuffers())
	{
		Diagnostics::Fatal(g_framePipelineLogger, __FILE__, __LINE__, "Ray-tracing frame shader resources are incomplete.");
	}
	if (frame.rayTracingScene.HasTraceableInstances() &&
	    (frame.sceneGpuData->RayTracing.InstanceCount == 0u || frame.sceneGpuData->RayTracing.MaterialCount == 0u))
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "Traceable SceneTlas instances have no matching hit-instance or material records.");
	}

	m_frameGraph->BindPersistentAccelerationStructure(
	    m_frameResources.SceneTlas,
	    frame.rayTracingScene.TlasResource,
	    frame.rayTracingScene.TlasGpuAddress);
}

void FramePipeline::BindSkyTexture(const FrameContext& frame)
{
	if (!frame.sceneData.sky.HasTexture())
	{
		Diagnostics::Fatal(
		    g_framePipelineLogger,
		    __FILE__,
		    __LINE__,
		    "TextureCache did not provide a valid sky texture before frame recording.");
	}

	const RendererTexture& skyTexture = *frame.sceneData.sky.texture;
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.External.Sky,
	    skyTexture.Resource,
	    skyTexture.ShaderResourceView,
	    FrameGraphTextureDesc::CreateColor("Sky", skyTexture.Width, skyTexture.Height, skyTexture.Format),
	    ResourceState::ShaderResource);
}

void FramePipeline::ExecuteFrameGraph(FrameContext& frame, RenderRayTracingScene* activeRayTracingScene)
{
	RenderHardwareInterface& renderHardwareInterface = m_rendererHost->GetRenderHardwareInterface();
	const RayTracedShadowSettings shadowSettings{
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get()};
	const bool rayTracedShadowsEnabled = CVarRayTracedShadowsEnabled.Get();

	m_frameGraph->Setup(frame);
	const FrameGraphPlan& compiledPlan = m_frameGraph->Compile();

	const RayTracingPassContext rayTracingPassContext{
	    .Scene = activeRayTracingScene,
	    .CapabilityReport = activeRayTracingScene != nullptr ? &activeRayTracingScene->GetCapabilities() : nullptr,
	    .ShadowSettings = &shadowSettings,
	    .ShadowsEnabled = rayTracedShadowsEnabled};
	const ImageProviderPassContext imageProviderPassContext = m_rendererHost->GetImageProviders().BuildPassContext();

	const PassRuntimeContext passRuntimeContext{
	    .HardwareInterface = renderHardwareInterface,
	    .PassRuntimes = m_rendererHost->GetRenderPassRuntimeCache(),
	    .PerFrame = m_perFrameData,
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
	deviceServices.SubmitFrame(m_renderInputConsumer->GetDynamicData().Metadata.FrameId);
	const RhiSubmissionToken graphicsToken = deviceServices.GetLastSubmittedToken(ERhiQueueType::Graphics);
	m_rendererHost->GetTextureCache().RecordUploadSubmission(graphicsToken);
	m_rendererHost->GetGpuMeshCache().RecordUploadSubmission(graphicsToken);
}

void FramePipeline::EndFrame() noexcept
{
	m_rendererHost->GetDeviceServices().AdvanceFrameInFlight();
}
