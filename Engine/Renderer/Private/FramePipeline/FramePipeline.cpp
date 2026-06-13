#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Camera/RenderCamera.h"
#include "Commands/RenderCommandContext.h"
#include "Config/RenderConfig.h"
#include "Core/Public/Diagnostics/LiveProfiler.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Frame/FrameContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Host/RendererSystemRoot.h"
#include "Pipeline/PipelineStateManager.h"
#include "RayTracing/RenderRayTracingPassServices.h"
#include "RayTracing/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/GameScene.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Upscaling/RenderUpscalingPassServices.h"
#include "Upscaling/UpscalerInputContractBuilder.h"
#include "Upscaling/UpscalerSubsystem.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(RendererSystemRoot& systems) noexcept :
    m_systems(&systems)
{
	m_frameExecutionDiagnostics.resize(RenderConfig::FramesInFlight);
	RenderDiagnostics& backendDiagnostics =
	    m_systems->GetRenderHardwareInterface().GetDiagnosticsService().GetDiagnostics();
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

std::uint64_t FramePipeline::ResolveRenderProductTextureId(RenderProductHandle handle) noexcept
{
	if (!handle || !m_frameGraph)
	{
		return 0;
	}

	const FrameGraphResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	return m_systems->GetRenderHardwareInterface().GetPresentationService().ResolveImGuiTextureId(
	    m_frameGraph->ResolveShaderResourceView(FrameGraphTextureHandle{resourceHandle}));
}

NativeResourceHandle FramePipeline::ResolveRenderProductResource(RenderProductHandle handle) const noexcept
{
	if (!handle || !m_frameGraph)
	{
		return NativeResourceHandle{};
	}

	const FrameGraphResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	return m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle});
}

void FramePipeline::TransitionRenderProduct(RenderProductHandle handle, ResourceState before, ResourceState after) noexcept
{
	if (!handle || !m_frameGraph)
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	const NativeResourceHandle resource = m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle});
	if (!resource)
	{
		return;
	}

	const ResourceState trackedBefore = m_frameGraph->GetTrackedResourceState(resourceHandle);
	const ResourceState resolvedBefore = trackedBefore != after ? trackedBefore : before;
	if (resolvedBefore == after)
	{
		return;
	}

	RenderHardwareInterface& renderHardware = m_systems->GetRenderHardwareInterface();
	RenderCommandList& commandList = renderHardware.GetGraphicsCommandList(renderHardware.GetCurrentFrameIndex());
	commandList.TransitionResource(resource, resolvedBefore, after);
	m_frameGraph->UpdateTrackedResourceState(resourceHandle, after);
}

std::uint32_t FramePipeline::GetLastUnresolvedBarrierWarningCount() const noexcept
{
	return m_frameGraph != nullptr ? m_frameGraph->GetLastUnresolvedBarrierWarningCount() : 0u;
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

bool FramePipeline::ShouldPresentSceneToBackBuffer() const noexcept
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
	    ShouldPresentSceneToBackBuffer()};

	FrameGraphFactory frameGraphFactory(dependencies);
	FrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphSceneExtent = dependencies.sceneExtent;

	m_frameProducts.SceneColor =
	    buildResult.SceneColor.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneColor.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_frameProducts.FinalSceneColor =
	    buildResult.FinalSceneColor.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.FinalSceneColor.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_frameProducts.SceneDepth =
	    buildResult.SceneDepth.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneDepth.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_frameProducts.MotionVectors =
	    buildResult.MotionVectors.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.MotionVectors.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_frameGraphSceneTlas = buildResult.SceneTlas;
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

	m_frameGraph.reset();
	InitializeFrameGraph(sceneExtent);
	if (UpscalerSubsystem* upscalerSubsystem = m_systems->GetUpscalerSubsystem())
	{
		upscalerSubsystem->OnResize(m_frameGraphSceneExtent, m_frameGraphSceneExtent);
	}
}

void FramePipeline::BeginFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.BeginFrame");
	RenderDeviceServices& backend = m_systems->GetBackend();
	TemporalDataBuilder& temporalDataBuilder = m_systems->GetTemporalDataBuilder();
	UpscalerSubsystem* upscalerSubsystem = m_systems->GetUpscalerSubsystem();

	if (m_bResizePending)
	{
		m_bResizePending = false;
		temporalDataBuilder.ResetHistory("Window resize");
		if (upscalerSubsystem != nullptr)
		{
			upscalerSubsystem->ResetHistory("Window resize");
		}

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
		if (upscalerSubsystem != nullptr)
		{
			upscalerSubsystem->ResetHistory("Scene extent changed");
		}
		RefreshFrameExecution(sceneExtent);
	}

	backend.BeginFrame();
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

	m_systems->GetSceneSnapshot().Capture(m_systems->GetGameScene().CaptureSnapshot());
	m_systems->GetTextureManager().LoadSceneTextures(m_systems->GetSceneSnapshot().textures);
	m_systems->GetRenderCamera().Update(m_systems->GetSceneSnapshot().camera);

	const RenderViewportExtent sceneExtent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();
	const std::uint32_t viewportWidth = sceneExtent.Width != 0u ? sceneExtent.Width : 1u;
	const std::uint32_t viewportHeight = sceneExtent.Height != 0u ? sceneExtent.Height : 1u;
	m_systems->GetBackend().UpdatePerFrameConstants(
	    static_cast<std::uint32_t>(CVarRenderViewMode.Get()),
	    viewportWidth,
	    viewportHeight);
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame end");
}

void FramePipeline::RefreshViewportRenderProducts() noexcept
{
	const RenderViewportExtent extent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();

	m_viewportRenderProducts.Clear();
	m_viewportRenderProducts.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{m_frameProducts.FinalSceneColor, extent, RenderProductFormat::ColorLdr});

	if (m_frameProducts.SceneDepth && HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{m_frameProducts.SceneDepth, extent, RenderProductFormat::DepthStencil});
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
	}

	FrameContext frame = [&]()
	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.BuildFrameContext");
		return BuildFrameContext(
		    m_systems->GetSceneSnapshot(),
		    renderHardwareInterface,
		    m_systems->GetRenderCamera(),
		    m_frameGraphSceneExtent,
		    m_systems->GetRenderSceneDataBuilder(),
		    m_systems->GetPerViewDataBuilder(),
		    m_systems->GetViewLightingBuilder(),
		    m_systems->GetTemporalDataBuilder());
	}();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame build context end");

	if (UpscalerSubsystem* upscalerSubsystem = m_systems->GetUpscalerSubsystem())
	{
		const UpscalerInputContract upscalerInputContract =
		    BuildUpscalerInputContract(
		        UpscalerInputContractBuildDesc{
		            .HudlessSceneColor = m_frameProducts.SceneColor,
		            .Depth = m_frameProducts.SceneDepth,
		            .MotionVectors = m_frameProducts.MotionVectors,
		            .FinalOutput = m_frameProducts.FinalSceneColor,
		            .RenderExtent = m_frameGraphSceneExtent,
		            .OutputExtent = m_frameGraphSceneExtent,
		            .FrameIndex = m_systems->GetTimer().GetFrameCount(),
		            .Camera = frame.mainView.perViewData.Camera,
		            .TemporalData = frame.mainView.perTemporalData,
		            .TemporalState = frame.mainView.temporalState});
		upscalerSubsystem->SetupFrame(upscalerInputContract);
	}

	if (m_frameGraph != nullptr && m_frameGraphSceneTlas.IsValid())
	{
		if (RenderRayTracingScene* renderRayTracingScene = m_systems->GetRenderRayTracingScene())
		{
			frame.rayTracingScene = renderRayTracingScene->Prepare(frame.sceneData);
			if (frame.rayTracingScene.HasBoundTlas())
			{
				m_frameGraph->BindPersistentAccelerationStructure(
				    m_frameGraphSceneTlas,
				    frame.rayTracingScene.TlasResource,
				    frame.rayTracingScene.TlasGpuAddress);
			}
			else
			{
				m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameGraphSceneTlas);
			}
		}
		else
		{
			m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameGraphSceneTlas);
		}
	}

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
	const RenderRayTracingPassServices rayTracingPassServices{
	    .Scene = m_systems->GetRenderRayTracingScene(),
	    .ShadowSettings = m_systems->GetRayTracedShadowSettings()};
	const RenderUpscalingPassServices upscalingPassServices{
	    .Subsystem = m_systems->GetUpscalerSubsystem()};
	const PassRuntimeServices passRuntimeServices{
	    .HardwareInterface = renderHardwareInterface,
	    .BackendDiagnostics = renderHardwareInterface.GetDiagnosticsService().GetDiagnostics(),
	    .RuntimeManager = m_systems->GetPipelineStateManager(),
	    .Textures = &m_systems->GetTextureManager(),
	    .RayTracing = &rayTracingPassServices,
	    .Upscaling = &upscalingPassServices};
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	auto gpuFrameScope =
	    frameDiagnostics.BeginGpuEvent(cmd, "GPU Frame", RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255});
	auto gpuFrameTimer = frameDiagnostics.BeginTimer(cmd, "GPU Frame");

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
	m_systems->GetBackend().AdvanceFrameInFlight();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::EndFrame end");
}

FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() noexcept
{
	return *m_frameExecutionDiagnostics[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

const FrameExecutionDiagnostics& FramePipeline::GetCurrentFrameDiagnostics() const noexcept
{
	return *m_frameExecutionDiagnostics[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

void FramePipeline::ReportResolvedTimings(std::uint32_t frameIndex, const FrameExecutionDiagnostics& frameDiagnostics) const noexcept
{
	const auto& resolvedTimers = frameDiagnostics.GetResolvedTimings();

	PublishLiveGpuTimings(resolvedTimers);

	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");

	if (rendererLogger == nullptr || !rendererLogger->should_log(spdlog::level::trace))
	{
		return;
	}

	if (resolvedTimers.empty())
	{
		return;
	}

	SPDLOG_LOGGER_TRACE(rendererLogger, "Resolved GPU timings for frame slot {} ({} scopes)", frameIndex, resolvedTimers.size());
	for (const ResolvedGpuTiming& resolvedTimer : resolvedTimers)
	{
		SPDLOG_LOGGER_TRACE(
		    rendererLogger,
		    "  {}: {:.3f} ms ({} ticks)",
		    resolvedTimer.Label,
		    resolvedTimer.DurationMilliseconds,
		    resolvedTimer.DurationTicks);
	}
}

void FramePipeline::PublishLiveGpuTimings(const std::vector<ResolvedGpuTiming>& resolvedTimers) const noexcept
{
	if (resolvedTimers.empty())
	{
		return;
	}

	Diagnostics::LiveProfiler& profiler = Diagnostics::LiveProfiler::Get();
	if (!profiler.IsEnabled())
	{
		return;
	}

	std::vector<Diagnostics::LiveProfiler::GpuTimingEntry> entries;
	entries.reserve(resolvedTimers.size());
	for (const ResolvedGpuTiming& resolvedTimer : resolvedTimers)
	{
		entries.push_back(
		    Diagnostics::LiveProfiler::GpuTimingEntry{
		        .Label = std::string_view(resolvedTimer.Label),
		        .DurationMicroseconds = static_cast<std::uint64_t>(resolvedTimer.DurationMilliseconds * 1000.0),
		        .Depth = resolvedTimer.Depth});
	}

	profiler.SubmitGpuFrame(entries.data(), entries.size());
}
