#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Camera/RenderCamera.h"
#include "Commands/RenderCommandContext.h"
#include "Frame/RhiFrameConstants.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Editor/EditorRenderPacketPlayer.h"
#include "Editor/EditorTextureRegistry.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Frame/Lighting/ReferenceLightingInvalidation.h"
#include "Frame/Lighting/RestirLightingInvalidation.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Host/RendererSystemRoot.h"
#include "Pipeline/PipelineStateManager.h"
#include "Providers/RendererImageProviderStack.h"
#include "Providers/ImageProviderFrameContext.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/RenderWorld.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Input/RenderInputConsumer.h"
#include "Meshes/GPUMeshCache.h"
#include "SceneData/RenderSceneGpuData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(RendererSystemRoot& systems) noexcept : m_systems(&systems)
{
	m_windowExtent = {
	    static_cast<std::uint32_t>(systems.GetWindow().GetWidth()),
	    static_cast<std::uint32_t>(systems.GetWindow().GetHeight())};
	m_renderInputConsumer = std::make_unique<RenderInputConsumer>(
	    systems.GetRenderWorld(), systems.GetBackend(), systems.GetGpuMeshCache(), systems.GetTextureManager(),
	    systems.GetMaterialCacheManager(), systems.GetRenderRayTracingScene());
	m_editorRenderPacketPlayer = std::make_unique<EditorRenderPacketPlayer>();
	m_editorTextureRegistry = std::make_unique<EditorTextureRegistry>();
	m_frameExecutionDiagnostics.resize(RhiFrameConstants::FramesInFlight);
	m_frameContexts.resize(RhiFrameConstants::FramesInFlight);
	RenderDiagnostics& backendDiagnostics = m_systems->GetRenderHardwareInterface().GetDiagnostics();
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}

	InitializeFrameGraph();
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

FramePipeline::~FramePipeline() noexcept = default;

std::uint64_t FramePipeline::RegisterEditorTexture(std::uint64_t nativeTextureId) noexcept
{
	return m_editorTextureRegistry->Register(nativeTextureId).Pack();
}

void FramePipeline::RequestResize(RenderViewportExtent extent, bool minimized) noexcept
{
	m_windowExtent = extent;
	m_windowMinimized = minimized;
	m_bResizePending = true;
}

void FramePipeline::RenderSerialUiFrame(
    const TimeInfo& timing,
    RendererSerialUiCallback composeUi,
    void* context) noexcept
{
	BeginFrame();
	SetupFrame(timing);
	RecordFrame();
	if (composeUi != nullptr) composeUi(context);
	SubmitFrame();
	EndFrame();
}

void FramePipeline::OnRender(
    const TimeInfo& timing,
    const EditorRenderPacket& editorUi) noexcept
{
	BeginFrame();
	SetupFrame(timing);
	RecordFrame();
	RefreshViewportEditorTexture();
	RenderEditorUi(editorUi);
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
	    .Render = m_systems->GetImageProviders().ResolveRenderExtent(outputExtent, imagePipeline),
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
	    m_systems->GetRenderHardwareInterface(),
	    m_systems->GetWindow(),
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
	m_imageProviderFrameGraphKey = m_systems->GetImageProviders().GetFrameGraphKey();
	m_frameGraph = std::move(buildResult.Graph);
}

void FramePipeline::RefreshFrameExecution() noexcept
{
	RefreshFrameExecution(ResolveFrameResolution());
}

void FramePipeline::RefreshFrameExecution(FrameResolutionExtents resolution) noexcept
{
	m_systems->GetBackend().WaitForIdle();
	RefreshFrameExecutionAfterDeviceIdle(resolution);
}

void FramePipeline::RefreshFrameExecutionAfterDeviceIdle(FrameResolutionExtents resolution) noexcept
{
	for (std::unique_ptr<FrameContext>& frameContext : m_frameContexts)
	{
		frameContext.reset();
	}

	m_frameGraph.reset();
	InitializeFrameGraph(resolution);
}

void FramePipeline::ResetTemporalState(std::string_view reason) noexcept
{
	InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	m_previousReferenceLightingHistoryInvalidationHash.reset();
	m_previousRestirLightingHistoryInvalidationHash.reset();
	m_systems->GetTemporalDataBuilder().ResetHistory(reason);
	m_systems->GetImageProviders().ResetHistory();
}

void FramePipeline::BeginFrame() noexcept
{
	RenderDeviceServices& backend = m_systems->GetBackend();
	(void) m_renderInputConsumer->ConsumePending();

	if (m_bResizePending)
	{
		m_bResizePending = false;
		ResetTemporalState("Window resize");

		if (!m_windowMinimized && m_windowExtent.IsValid())
		{
			backend.ResizeSwapChain();
			RefreshFrameExecutionAfterDeviceIdle(ResolveFrameResolution());
		}
	}

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

	const ImageProviderGraphKey imageProviderFrameGraphKey = m_systems->GetImageProviders().GetFrameGraphKey();
	if (imageProviderFrameGraphKey != m_imageProviderFrameGraphKey)
	{
		backend.WaitForIdle();
		m_systems->RefreshImageProviders();
		ResetTemporalState("Image provider graph mode changed");
		RefreshFrameExecutionAfterDeviceIdle(ResolveFrameResolution());
		m_imageProviderFrameGraphKey = imageProviderFrameGraphKey;
	}

	backend.BeginFrame();
	m_frameContexts[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()].reset();
	m_systems->TickDiagnostics(m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex());
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
}

void FramePipeline::SetupFrame(const TimeInfo& timing) noexcept
{
	RefreshViewportRenderProducts();

	RenderDeviceServices& backend = m_systems->GetBackend();
	RenderCommandList& graphicsCommandList = backend.GetCurrentGraphicsCommandList();
	TextureManager& textureManager = m_systems->GetTextureManager();
	const bool useCopyQueue = textureManager.HasPendingSceneTextureUploads(m_systems->GetRenderWorld().GetTextures()) &&
	                          m_systems->GetRenderHardwareInterface().GetCapabilities().Queues.SupportsIndependent(
	                              ERhiQueueType::Copy);
	RenderCommandList& uploadCommandList =
	    useCopyQueue ? backend.BeginCommandList(ERhiQueueType::Copy) : graphicsCommandList;
	const std::vector<RhiResourceHandle> uploadedResources =
	    textureManager.LoadSceneTextures(m_systems->GetRenderWorld().GetTextures(), uploadCommandList);
	if (useCopyQueue)
	{
		const RhiSubmissionToken uploadToken = backend.SubmitCommandList(uploadCommandList);
		backend.QueueWait(ERhiQueueType::Graphics, uploadToken);
		for (const RhiResourceHandle resource : uploadedResources)
		{
			graphicsCommandList.TransitionResource(resource, ResourceState::Common, ResourceState::ShaderResource);
		}
	}
	m_systems->GetRenderCamera().Update(m_renderInputConsumer->GetDynamicData().Camera);

	const RenderViewportExtent renderExtent =
	    m_frameGraphRenderExtent.IsValid() ? m_frameGraphRenderExtent : ResolveFrameResolution().Render;
	m_perFrameData = m_perFrameDataBuilder.Build(timing, CVarRenderViewMode.Get(), renderExtent);
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

void FramePipeline::RefreshViewportEditorTexture() noexcept
{
	const ViewportPresentationProduct presentation =
	    BeginViewportPresentation(RenderOutputFlags::SceneColor);
	if (!presentation)
	{
		return;
	}

	RenderProduct product = presentation.Product;
	product.EditorTextureHandle = m_editorTextureRegistry
	                                  ->PublishViewportTexture(
	                                      presentation.TextureId,
	                                      m_viewportRenderProducts.GetGeneration())
	                                  .Pack();
	m_viewportRenderProducts.SetProduct(RenderOutputFlags::SceneColor, product);
	EndViewportPresentation(RenderOutputFlags::SceneColor);
}

void FramePipeline::RenderEditorUi(const EditorRenderPacket& packet) noexcept
{
	if (!packet.HasDrawData() ||
	    packet.ViewportGeneration != m_viewportRenderProducts.GetGeneration())
	{
		return;
	}

	(void) BeginViewportPresentation(RenderOutputFlags::SceneColor);
	constexpr float clearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	RhiPresentationService& presentation =
	    m_systems->GetRenderHardwareInterface().GetPresentationService();
	presentation.BeginPresentRenderPass(clearColor);
	m_editorRenderPacketPlayer->Render(
	    packet,
	    *m_editorTextureRegistry,
	    m_systems->GetImGuiRenderer());
	presentation.EndPresentRenderPass();
	EndViewportPresentation(RenderOutputFlags::SceneColor);
}

void FramePipeline::RecordFrame() noexcept
{
	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	const RayTracedShadowSettings shadowSettings{
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get()};
	const bool rayTracedShadowsEnabled = CVarRayTracedShadowsEnabled.Get();
	RenderRayTracingScene* activeRayTracingScene = m_systems->GetRenderRayTracingScene();
	const RenderFrameDynamicData& dynamic = m_renderInputConsumer->GetDynamicData();
	if (dynamic.Metadata.ResetHistory || m_systems->GetRenderWorld().ConsumeHistoryReset())
	{
		ResetTemporalState(dynamic.Metadata.CameraCut ? "Render input camera cut" : "Render input generation reset");
	}

	std::unique_ptr<FrameContext>& frameSlot = m_frameContexts[renderHardwareInterface.GetCurrentFrameIndex()];
	frameSlot = [&]()
	{
		return std::make_unique<FrameContext>(BuildFrameContext(
		    m_systems->GetRenderWorld(),
		    dynamic,
		    renderHardwareInterface.GetResourceService(),
		    m_systems->GetRenderCamera(),
		    m_frameGraphRenderExtent,
		    m_systems->GetRenderSceneDataBuilder(),
		    activeRayTracingScene,
		    m_systems->GetPerViewDataBuilder(),
		    m_systems->GetTemporalDataBuilder()));
	}();
	FrameContext& frame = *frameSlot;
	if (GetLightingMode() == LightingMode::RestirPathTraced)
	{
		const std::uint64_t invalidationHash = BuildRestirLightingHistoryInvalidationHash(frame);
		const bool historyInvalidationRequired = !m_previousRestirLightingHistoryInvalidationHash ||
		                                         invalidationHash != *m_previousRestirLightingHistoryInvalidationHash;
		if (historyInvalidationRequired)
		{
			InvalidateRestirLightingHistory(*m_frameGraph, m_frameResources.History);
			m_systems->GetImageProviders().ResetHistory();
		}
		m_previousRestirLightingHistoryInvalidationHash = invalidationHash;
	}
	else if (GetLightingMode() == LightingMode::ReferencePathTraced)
	{
		const std::uint64_t invalidationHash = BuildReferenceLightingHistoryInvalidationHash(frame);
		const bool historyInvalidationRequired = !m_previousReferenceLightingHistoryInvalidationHash ||
		                                         invalidationHash != *m_previousReferenceLightingHistoryInvalidationHash;
		if (historyInvalidationRequired)
		{
			m_frameGraph->InvalidateTextureHistory(m_frameResources.History.ReferenceLighting);
		}
		m_previousReferenceLightingHistoryInvalidationHash = invalidationHash;
	}
	if (frame.mainView.perTemporalData.HistoryValid == 0u)
	{
		InvalidateFrameHistory(*m_frameGraph, m_frameResources.History);
	}
	m_systems->GetImageProviders().SetupFrame(
	    ImageProviderFrameContext{
	        .RenderExtent = m_frameGraphRenderExtent,
	        .OutputExtent = m_frameGraphOutputExtent,
	        .FrameId = dynamic.Metadata.FrameId,
	        .ProviderGeneration = dynamic.Metadata.ProviderGeneration,
	        .Camera = frame.mainView.perViewData.Camera,
	        .TemporalData = frame.mainView.perTemporalData,
	        .TemporalState = frame.mainView.temporalState,
	        .ResetHistory = frame.mainView.perTemporalData.HistoryValid == 0u});

	if (m_frameGraph != nullptr && m_frameResources.SceneTlas.IsValid())
	{
		if (activeRayTracingScene != nullptr)
		{
			frame.rayTracingScene = activeRayTracingScene->Prepare(frame.sceneData);
			if (frame.rayTracingScene.HasBoundTlas())
			{
				m_frameGraph->BindPersistentAccelerationStructure(
				    m_frameResources.SceneTlas,
				    frame.rayTracingScene.TlasResource,
				    frame.rayTracingScene.TlasGpuAddress);
			}
			else
			{
				m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameResources.SceneTlas);
			}
		}
		else
		{
			m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameResources.SceneTlas);
		}
	}

	if (frame.sceneData.sky.HasTexture())
	{
		const RendererTexture& skyTexture = *frame.sceneData.sky.texture;
		m_frameGraph->BindPersistentTexture(
		    m_frameResources.External.Sky,
		    skyTexture.Resource,
		    skyTexture.ShaderResourceView,
		    FrameGraphTextureDesc::CreateColor(
		        "Sky",
		        skyTexture.Width,
		        skyTexture.Height,
		        skyTexture.Format),
		    ResourceState::ShaderResource);
	}
	else
	{
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.External.Sky);
	}
	BindRenderSceneGpuResources(*m_frameGraph, m_frameResources.External.Scene, frame.sceneGpuData);
	m_frameGraph->Setup(frame);
	const FrameGraphPlan& compiledPlan = m_frameGraph->Compile();
	const RenderRayTracingPassServices rayTracingPassServices{
	    .Scene = activeRayTracingScene,
	    .CapabilityReport = activeRayTracingScene != nullptr ? &activeRayTracingScene->GetCapabilities() : nullptr,
	    .ShadowSettings = &shadowSettings,
	    .ShadowsEnabled = rayTracedShadowsEnabled};
	const RendererImageProviderPassServices imageProviderPassServices = m_systems->GetImageProviders().BuildPassServices();
	const PassRuntimeServices passRuntimeServices{
	    .HardwareInterface = renderHardwareInterface,
	    .RuntimeManager = m_systems->GetPipelineStateManager(),
	    .PerFrame = m_perFrameData,
	    .History = ResolveFrameHistoryValidity(*m_frameGraph, m_frameResources.History),
	    .Textures = &m_systems->GetTextureManager(),
	    .RayTracing = &rayTracingPassServices,
	    .ImageProviders = &imageProviderPassServices};
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	m_frameGraph->Execute(compiledPlan, m_systems->GetBackend(), frame, passRuntimeServices, frameDiagnostics);
}

void FramePipeline::SubmitFrame() noexcept
{
	m_systems->GetBackend().SubmitFrame();
}

void FramePipeline::EndFrame() noexcept
{
	m_systems->GetBackend().AdvanceFrameInFlight();
}
