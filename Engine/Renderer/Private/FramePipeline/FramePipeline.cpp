#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Camera/RenderCamera.h"
#include "Commands/RenderCommandContext.h"
#include "Frame/RhiFrameConstants.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Frame/Lighting/IndirectReconstruction.h"
#include "Frame/Presentation/Upscaling.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Host/RendererSystemRoot.h"
#include "Pipeline/PipelineStateManager.h"
#include "Providers/RendererImageProviderStack.h"
#include "RayTracing/Effects/IndirectDiffuse/IndirectDiffuseSettings.h"
#include "RayTracing/Effects/IndirectSpecular/IndirectSpecularSettings.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/GameScene.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(RendererSystemRoot& systems) noexcept :
    m_systems(&systems)
{
	m_frameExecutionDiagnostics.resize(RhiFrameConstants::FramesInFlight);
	m_frameContexts.resize(RhiFrameConstants::FramesInFlight);
	RenderDiagnostics& backendDiagnostics =
	    m_systems->GetRenderHardwareInterface().GetDiagnostics();
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}

	InitializeFrameGraph();
	CreateExposureHistoryResources();
	BindWindowResizeEvent();
}

FramePipeline::~FramePipeline() noexcept
{
	ReleaseExposureHistoryResources();
}

void FramePipeline::PrepareHostFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.PrepareHostFrame");
	BeginFrame();
	SetupFrame();
}

void FramePipeline::RecordHostFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RecordHostFrame");
	RecordFrame();
}

void FramePipeline::SubmitHostFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SubmitHostFrame");
	SubmitFrame();
	EndFrame();
}

void FramePipeline::OnRender() noexcept
{
	PrepareHostFrame();
	RecordHostFrame();
	SubmitHostFrame();
}

RenderViewportExtent FramePipeline::ResolveSceneExtent() const noexcept
{
	const Window& window = m_systems->GetWindow();
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return RenderViewportExtent{static_cast<std::uint32_t>(window.GetWidth()), static_cast<std::uint32_t>(window.GetHeight())};
}

bool FramePipeline::ShouldOutputToBackBuffer() const noexcept
{
	return m_viewportRenderRequest.ViewportId == 0;
}

void FramePipeline::InitializeFrameGraph() noexcept
{
	InitializeFrameGraph(ResolveSceneExtent());
}

void FramePipeline::InitializeFrameGraph(RenderViewportExtent sceneExtent) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeFrameGraph");
	const FrameGraphDependencies dependencies{
	    m_systems->GetRenderHardwareInterface(),
	    m_systems->GetWindow(),
	    sceneExtent,
	    ShouldOutputToBackBuffer()};

	FrameGraphFactory frameGraphFactory(dependencies);
	FrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphSceneExtent = dependencies.sceneExtent;
	m_frameResources = buildResult.Resources;
	m_renderPath = buildResult.RenderPath;
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
	RefreshFrameExecution(ResolveSceneExtent());
}

void FramePipeline::RefreshFrameExecution(RenderViewportExtent sceneExtent) noexcept
{
	RenderDeviceServices& backend = m_systems->GetBackend();
	backend.Flush();
	for (std::unique_ptr<FrameContext>& frameContext : m_frameContexts)
	{
		frameContext.reset();
	}

	m_frameGraph.reset();
	InitializeFrameGraph(sceneExtent);
	m_systems->GetImageProviders().OnResize(m_frameGraphSceneExtent, m_frameGraphSceneExtent);
}

void FramePipeline::BeginFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.BeginFrame");
	RenderDeviceServices& backend = m_systems->GetBackend();
	TemporalDataBuilder& temporalDataBuilder = m_systems->GetTemporalDataBuilder();

	if (m_bResizePending)
	{
		m_bResizePending = false;
		temporalDataBuilder.ResetHistory("Window resize");
		ResetExposureHistory();
		m_systems->GetImageProviders().ResetHistory("Window resize");

		if (m_systems->GetWindow().HasValidSize())
		{
			backend.Flush();
			backend.ResizeSwapChain();
			RefreshFrameExecution(ResolveSceneExtent());
		}
	}

	const RenderViewportExtent sceneExtent = ResolveSceneExtent();
	if (sceneExtent.Width != m_frameGraphSceneExtent.Width || sceneExtent.Height != m_frameGraphSceneExtent.Height)
	{
		temporalDataBuilder.ResetHistory("Scene extent changed");
		ResetExposureHistory();
		m_systems->GetImageProviders().ResetHistory("Scene extent changed");
		RefreshFrameExecution(sceneExtent);
	}

	const FrameRenderPath renderPath = ResolveFrameRenderPathFromSettings();
	if (renderPath != m_renderPath)
	{
		temporalDataBuilder.ResetHistory("Render path changed");
		ResetExposureHistory();
		m_systems->GetImageProviders().ResetHistory("Render path changed");
		RefreshFrameExecution(sceneExtent);
	}

	const std::uint32_t imageProviderFrameGraphKey = m_systems->GetImageProviders().GetFrameGraphKey();
	if (imageProviderFrameGraphKey != m_imageProviderFrameGraphKey)
	{
		temporalDataBuilder.ResetHistory("Image provider graph mode changed");
		ResetExposureHistory();
		m_systems->RefreshImageProviders();
		RefreshFrameExecution(sceneExtent);
		m_systems->GetImageProviders().ResetHistory("Image provider graph mode changed");
		m_imageProviderFrameGraphKey = imageProviderFrameGraphKey;
	}

	backend.BeginFrame();
	m_frameContexts[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()].reset();
	m_systems->TickDiagnostics(m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex());
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
	ReportResolvedTimings(m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex(), frameDiagnostics);
}

void FramePipeline::SetupFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SetupFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame begin");

	Timer& timer = m_systems->GetTimer();
	timer.Tick();
	RefreshViewportRenderProducts();

	m_sceneSnapshot.Capture(m_systems->GetGameScene().CaptureSnapshot());
	m_systems->GetTextureManager().LoadSceneTextures(m_sceneSnapshot.textures);
	m_systems->GetRenderCamera().Update(m_sceneSnapshot.camera);

	const RenderViewportExtent sceneExtent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();
	m_perFrameData = m_perFrameDataBuilder.Build(timer, CVarRenderViewMode.Get(), sceneExtent);
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame end");
}

void FramePipeline::RefreshViewportRenderProducts() noexcept
{
	const RenderViewportExtent extent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();

	m_viewportRenderProducts.Clear();
	m_viewportRenderProducts.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{ToRenderProductHandle(m_frameResources.ViewportProducts.FinalSceneColor), extent, RenderProductFormat::ColorLdr});

	if (m_frameResources.ViewportProducts.SceneDepth.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{ToRenderProductHandle(m_frameResources.ViewportProducts.SceneDepth), extent, RenderProductFormat::DepthStencil});
	}

	if (m_frameResources.ViewportProducts.Normals.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::Normals))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::Normals,
		    RenderProduct{ToRenderProductHandle(m_frameResources.ViewportProducts.Normals), extent, RenderProductFormat::ColorHdr});
	}
}

void FramePipeline::RecordFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RecordFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame build context begin");

	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	std::string temporalResetReason;
	SceneRenderStateCoordinator* sceneRenderStateCoordinator = m_systems->GetSceneRenderStateCoordinator();
	if (sceneRenderStateCoordinator != nullptr && sceneRenderStateCoordinator->ConsumeTemporalHistoryResetRequest(temporalResetReason))
	{
		m_systems->GetTemporalDataBuilder().ResetHistory(temporalResetReason);
		ResetExposureHistory();
	}

	std::unique_ptr<FrameContext>& frameSlot = m_frameContexts[renderHardwareInterface.GetCurrentFrameIndex()];
	frameSlot = [&]()
	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.BuildFrameContext");
		return std::make_unique<FrameContext>(
		    BuildFrameContext(
		        m_sceneSnapshot,
		        renderHardwareInterface,
		        m_systems->GetRenderCamera(),
		        m_frameGraphSceneExtent,
		        m_systems->GetRenderSceneDataBuilder(),
		        m_systems->GetRenderRayTracingScene(),
		        m_systems->GetPerViewDataBuilder(),
		        m_systems->GetTemporalDataBuilder()));
	}();
	FrameContext& frame = *frameSlot;
	if (frame.mainView.perTemporalData.HistoryValid == 0u)
	{
		ResetExposureHistory();
	}
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame build context end");

	if (m_frameResources.UpscalerProviderInputs.ScalingInputColor.IsValid())
	{
		const UpscalerInputContract upscalerInputContract = BuildFrameUpscalerInputContract(
		    m_frameResources.UpscalerProviderInputs,
		    m_frameGraphSceneExtent,
		    m_systems->GetTimer().GetFrameCount(),
		    frame.mainView.perViewData.Camera,
		    frame.mainView.perTemporalData,
		    frame.mainView.temporalState);
		m_systems->GetImageProviders().SetupUpscalerFrame(upscalerInputContract);
	}

	if (m_frameResources.RayReconstructionProviderInputs.NoisyInputColor.IsValid())
	{
		const RayReconstructionInputContract reconstructionInputContract = BuildFrameIndirectRayReconstructionInputContract(
		    m_frameResources.RayReconstructionProviderInputs,
		    m_frameGraphSceneExtent,
		    m_systems->GetTimer().GetFrameCount(),
		    frame.mainView.perViewData.Camera,
		    frame.mainView.perTemporalData,
		    frame.mainView.temporalState);
		m_systems->GetImageProviders().SetupRayReconstructionFrame(reconstructionInputContract);
	}

	if (m_frameGraph != nullptr && m_frameResources.Persistent.SceneTlas.IsValid())
	{
		if (RenderRayTracingScene* renderRayTracingScene = m_systems->GetRenderRayTracingScene())
		{
			frame.rayTracingScene = renderRayTracingScene->Prepare(frame.sceneData);
			if (frame.rayTracingScene.HasBoundTlas())
			{
				m_frameGraph->BindPersistentAccelerationStructure(
				    m_frameResources.Persistent.SceneTlas,
				    frame.rayTracingScene.TlasResource,
				    frame.rayTracingScene.TlasGpuAddress);
				BindRayTracingFrameGraphResources(frame.rayTracingScene);
			}
			else
			{
				m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameResources.Persistent.SceneTlas);
				ClearRayTracingFrameGraphResources();
			}
		}
		else
		{
			m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameResources.Persistent.SceneTlas);
			ClearRayTracingFrameGraphResources();
		}
	}

	BindExposureHistoryFrameGraphResources();

	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.FrameGraphSetup");
		m_frameGraph->Setup(frame);
	}
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph setup end");

	const FrameGraphPlan compiledPlan = [&]()
	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.FrameGraphCompile");
		return m_frameGraph->Compile();
	}();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph compile end (passes={})", compiledPlan.executionOrder.size());
	RenderCommandList& commandList = m_systems->GetBackend().GetCurrentGraphicsCommandList();
	RenderCommandContext cmd(commandList);
	const IndirectDiffuseSettings indirectDiffuseSettings = BuildIndirectDiffuseSettingsFromCVars();
	const IndirectSpecularSettings indirectSpecularSettings = BuildIndirectSpecularSettingsFromCVars();
	const RenderRayTracingPassServices rayTracingPassServices{
	    .Scene = m_systems->GetRenderRayTracingScene(),
	    .CapabilityReport = m_systems->GetRenderRayTracingScene() != nullptr
	                            ? &m_systems->GetRenderRayTracingScene()->GetCapabilities()
	                            : nullptr,
	    .ShadowSettings = m_systems->GetRayTracedShadowSettings(),
	    .IndirectDiffuseSettings = &indirectDiffuseSettings,
	    .IndirectSpecularSettings = &indirectSpecularSettings};
	const RendererImageProviderPassServices imageProviderPassServices = m_systems->GetImageProviders().BuildPassServices();
	const PassRuntimeServices passRuntimeServices{
	    .HardwareInterface = renderHardwareInterface,
	    .RuntimeManager = m_systems->GetPipelineStateManager(),
	    .PerFrame = m_perFrameData,
	    .ExposureHistoryValid = m_exposureHistoryValid,
	    .Textures = &m_systems->GetTextureManager(),
	    .RayTracing = &rayTracingPassServices,
	    .ImageProviders = &imageProviderPassServices};
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	auto gpuFrameScope =
	    CVarRendererDiagnosticMarkerVerbosity.Get() != RendererDiagnosticMarkerVerbosity::Off
	        ? frameDiagnostics.BeginGpuScope(
	              cmd,
	              "GPU Frame",
	              RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255})
	        : ScopedGpuScope{};

	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph execute begin");
	m_frameGraph->Execute(compiledPlan, cmd, frame, passRuntimeServices, frameDiagnostics);

	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph execute end");
}

void FramePipeline::SubmitFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SubmitFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SubmitFrame begin");
	m_systems->GetBackend().SubmitFrame();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SubmitFrame end");
}

void FramePipeline::EndFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.EndFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::EndFrame begin");
	m_exposureHistoryValid = HasExposureHistoryResources();
	m_systems->GetBackend().AdvanceFrameInFlight();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::EndFrame end");
}
