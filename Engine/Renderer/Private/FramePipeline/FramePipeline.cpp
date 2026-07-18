#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Camera/RenderCamera.h"
#include "Commands/RenderCommandContext.h"
#include "Frame/RhiFrameConstants.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
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
#include "World/GameWorld.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"
#include "SceneData/RenderSceneGpuData.h"
#include "Textures/RendererTexture.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(RendererSystemRoot& systems) noexcept : m_systems(&systems)
{
	m_frameExecutionDiagnostics.resize(RhiFrameConstants::FramesInFlight);
	m_frameContexts.resize(RhiFrameConstants::FramesInFlight);
	RenderDiagnostics& backendDiagnostics = m_systems->GetRenderHardwareInterface().GetDiagnostics();
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}

	InitializeFrameGraph();
	BindWindowResizeEvent();
}

FramePipeline::~FramePipeline() noexcept = default;

void FramePipeline::PrepareHostFrame() noexcept
{
	BeginFrame();
	SetupFrame();
}

void FramePipeline::RecordHostFrame() noexcept
{
	RecordFrame();
}

void FramePipeline::SubmitHostFrame() noexcept
{
	SubmitFrame();
	EndFrame();
}

void FramePipeline::OnRender() noexcept
{
	PrepareHostFrame();
	RecordHostFrame();
	SubmitHostFrame();
}

RenderViewportExtent FramePipeline::ResolveOutputExtent() const noexcept
{
	const Window& window = m_systems->GetWindow();
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return RenderViewportExtent{static_cast<std::uint32_t>(window.GetWidth()), static_cast<std::uint32_t>(window.GetHeight())};
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

void FramePipeline::BindWindowResizeEvent() noexcept
{
	auto handle = m_systems->GetWindow().OnResized.Add(
	    [this]()
	    {
		    m_bResizePending = true;
	    });
	m_resizeHandle = ScopedEventHandle(m_systems->GetWindow().OnResized, handle);
}

void FramePipeline::RefreshFrameExecution() noexcept
{
	RefreshFrameExecution(ResolveFrameResolution());
}

void FramePipeline::RefreshFrameExecution(FrameResolutionExtents resolution) noexcept
{
	RenderDeviceServices& backend = m_systems->GetBackend();
	backend.WaitForIdle();
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

	if (m_bResizePending)
	{
		m_bResizePending = false;
		ResetTemporalState("Window resize");

		if (m_systems->GetWindow().HasValidSize())
		{
			backend.WaitForIdle();
			backend.ResizeSwapChain();
			RefreshFrameExecution(ResolveFrameResolution());
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
		m_systems->RefreshImageProviders();
		ResetTemporalState("Image provider graph mode changed");
		RefreshFrameExecution(ResolveFrameResolution());
		m_imageProviderFrameGraphKey = imageProviderFrameGraphKey;
	}

	backend.BeginFrame();
	m_frameContexts[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()].reset();
	m_systems->TickDiagnostics(m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex());
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
}

void FramePipeline::SetupFrame() noexcept
{
	Timer& timer = m_systems->GetTimer();
	timer.Tick();
	RefreshViewportRenderProducts();

	m_sceneSnapshot.Capture(m_systems->GetGameWorld().CaptureSnapshot());
	RenderDeviceServices& backend = m_systems->GetBackend();
	RenderCommandList& graphicsCommandList = backend.GetCurrentGraphicsCommandList();
	TextureManager& textureManager = m_systems->GetTextureManager();
	const bool useCopyQueue = textureManager.HasPendingSceneTextureUploads(m_sceneSnapshot.textures) &&
	                          m_systems->GetRenderHardwareInterface().GetCapabilities().Queues.SupportsIndependent(
	                              ERhiQueueType::Copy);
	RenderCommandList& uploadCommandList =
	    useCopyQueue ? backend.BeginCommandList(ERhiQueueType::Copy) : graphicsCommandList;
	const std::vector<RhiResourceHandle> uploadedResources =
	    textureManager.LoadSceneTextures(m_sceneSnapshot.textures, uploadCommandList);
	if (useCopyQueue)
	{
		const RhiSubmissionToken uploadToken = backend.SubmitCommandList(uploadCommandList);
		backend.QueueWait(ERhiQueueType::Graphics, uploadToken);
		for (const RhiResourceHandle resource : uploadedResources)
		{
			graphicsCommandList.TransitionResource(resource, ResourceState::Common, ResourceState::ShaderResource);
		}
	}
	m_systems->GetRenderCamera().Update(m_sceneSnapshot.camera);

	const RenderViewportExtent renderExtent =
	    m_frameGraphRenderExtent.IsValid() ? m_frameGraphRenderExtent : ResolveFrameResolution().Render;
	m_perFrameData = m_perFrameDataBuilder.Build(timer, CVarRenderViewMode.Get(), renderExtent);
}

void FramePipeline::RefreshViewportRenderProducts() noexcept
{
	const FrameResolutionExtents resolution =
	    m_frameGraphOutputExtent.IsValid() && m_frameGraphRenderExtent.IsValid()
	        ? FrameResolutionExtents{.Render = m_frameGraphRenderExtent, .Output = m_frameGraphOutputExtent}
	        : ResolveFrameResolution();

	m_viewportRenderProducts.Clear();
	m_viewportRenderProducts.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{
	        ToRenderProductHandle(m_frameResources.ViewportProducts.FinalSceneColor),
	        resolution.Output,
	        RenderProductFormat::ColorLdr});

	if (m_frameResources.ViewportProducts.SceneDepth.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{
		        ToRenderProductHandle(m_frameResources.ViewportProducts.SceneDepth),
		        resolution.Render,
		        RenderProductFormat::Float});
	}

	if (m_frameResources.ViewportProducts.Normals.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::Normals))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::Normals,
		    RenderProduct{
		        ToRenderProductHandle(m_frameResources.ViewportProducts.Normals),
		        resolution.Render,
		        RenderProductFormat::ColorHdr});
	}
}

void FramePipeline::RecordFrame() noexcept
{
	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	const RayTracedShadowSettings shadowSettings{
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get()};
	const bool rayTracedShadowsEnabled = CVarRayTracedShadowsEnabled.Get();
	RenderRayTracingScene* activeRayTracingScene = m_systems->GetRenderRayTracingScene();
	std::string temporalResetReason;
	SceneRenderStateCoordinator* sceneRenderStateCoordinator = m_systems->GetSceneRenderStateCoordinator();
	if (sceneRenderStateCoordinator != nullptr && sceneRenderStateCoordinator->ConsumeTemporalHistoryResetRequest(temporalResetReason))
	{
		ResetTemporalState(temporalResetReason);
	}

	std::unique_ptr<FrameContext>& frameSlot = m_frameContexts[renderHardwareInterface.GetCurrentFrameIndex()];
	frameSlot = [&]()
	{
		return std::make_unique<FrameContext>(BuildFrameContext(
		    m_sceneSnapshot,
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
	        .FrameIndex = m_systems->GetTimer().GetFrameCount(),
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
