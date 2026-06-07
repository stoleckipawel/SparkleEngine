#include "PCH.h"
#include "Renderer.h"

#include "Level/LevelManager.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"
#include "Textures/TextureManager.h"
#include "Meshes/GPUMeshCache.h"
#include "Scene/GameScene.h"
#include "Time/Timer.h"
#include "Camera/RenderCamera.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Config/RenderConfig.h"
#include "Commands/RenderCommandContext.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Diagnostics/MeshDiagnosticsCollector.h"
#include "Diagnostics/RendererMemoryMonitor.h"
#include "Core/Public/Diagnostics/LiveProfiler.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Frame/FrameContext.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Scene/Camera/CameraComponent.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/CookedMesh.h"
#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/SceneMeshes.h"

#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Pipeline/PipelineStateManager.h"
#include "RayTracing/RayTracedShadowSettings.h"
#include "RayTracing/RayTracingCapabilityReport.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"

Renderer::Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept :
    m_timer(&timer), m_gameScene(&gameScene), m_window(&window)
{
	InitializeCoreSystems();

	InitializeSceneSystems(levelManager);
	InitializeFrameGraph();
	BindWindowResizeEvent();

	PostLoad();
}

RenderHardwareInterface& Renderer::GetRenderHardwareInterface() noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

const RenderHardwareInterface& Renderer::GetRenderHardwareInterface() const noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

RhiImGuiRenderer& Renderer::GetImGuiRenderer() noexcept
{
	return m_backend->GetImGuiRenderer();
}

CookedShaderReloadResult Renderer::ReloadCookedShaders() noexcept
{
	if (m_pipelineStateManager != nullptr)
	{
		return m_pipelineStateManager->ReloadCookedShaders();
	}

	return CookedShaderReloadResult::Failure("Renderer has no pipeline state manager; cooked shader reload was skipped.");
}

std::uint64_t Renderer::GetShaderPackageGeneration() const noexcept
{
	return m_pipelineStateManager != nullptr ? m_pipelineStateManager->GetShaderPackageGeneration() : 0;
}

MeshDiagnosticsSnapshot Renderer::CaptureMeshDiagnostics() const
{
	if (m_gameScene == nullptr)
	{
		return MeshDiagnosticsSnapshot{};
	}

	return MeshDiagnosticsCollector::Capture(m_gameScene->GetMeshes(), m_gpuMeshCache.get());
}

TextureDiagnosticsSnapshot Renderer::CaptureTextureDiagnostics() const
{
	return m_textureManager != nullptr ? m_textureManager->CaptureDiagnosticsSnapshot() : TextureDiagnosticsSnapshot{};
}

RendererMemoryDiagnosticsSnapshot Renderer::CaptureMemoryDiagnostics() const
{
	return m_memoryMonitor != nullptr ? m_memoryMonitor->GetLatestSnapshot() : RendererMemoryDiagnosticsSnapshot{};
}

void Renderer::PrepareHostFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.PrepareHostFrame");
	BeginFrame();
	SetupFrame();
}

void Renderer::RecordHostFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RecordHostFrame");
	RecordFrame();
}

void Renderer::SubmitHostFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SubmitHostFrame");
	SubmitFrame();
	EndFrame();
}

std::uint64_t Renderer::ResolveRenderProductTextureId(RenderProductHandle handle) const noexcept
{
	if (!handle || !m_frameGraph)
	{
		return 0;
	}

	const FrameGraphResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	return m_frameGraph->ResolveShaderResourceView(FrameGraphTextureHandle{resourceHandle}).Value;
}

NativeResourceHandle Renderer::ResolveRenderProductResource(RenderProductHandle handle) const noexcept
{
	if (!handle || !m_frameGraph)
	{
		return NativeResourceHandle{};
	}

	const FrameGraphResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	return m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle});
}

void Renderer::TransitionRenderProduct(RenderProductHandle handle, ResourceState before, ResourceState after) noexcept
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

	RenderHardwareInterface& renderHardware = GetRenderHardwareInterface();
	RenderCommandList& commandList = renderHardware.GetGraphicsCommandList(renderHardware.GetCurrentFrameIndex());
	commandList.TransitionResource(resource, resolvedBefore, after);
	m_frameGraph->UpdateTrackedResourceState(resourceHandle, after);
}

void Renderer::InitializeCoreSystems() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeCoreSystems");

	{
		SPARKLE_CPU_SCOPE("Renderer.CreateBackend");
		m_backend = RenderDeviceServices::Create(*m_timer, *m_window);
	}
	{
		SPARKLE_CPU_SCOPE("Renderer.CreatePipelineStateManager");
		m_pipelineStateManager = std::make_unique<PipelineStateManager>(GetRenderHardwareInterface());
	}
	{
		SPARKLE_CPU_SCOPE("Renderer.CreateGPUMeshCache");
		m_gpuMeshCache = std::make_unique<GPUMeshCache>(GetRenderHardwareInterface());
	}

	RenderDiagnostics& backendDiagnostics = GetRenderHardwareInterface().GetDiagnostics();
	const RayTracingCapabilityReport rayTracingCapabilities =
	    RayTracingCapabilityReporter::Build(GetRenderHardwareInterface().GetCapabilities());
	RayTracingCapabilityReporter::LogOnce(rayTracingCapabilities);
	LogRayTracedShadowSettingsOnce(BuildRayTracedShadowSettingsFromCVars(), rayTracingCapabilities);

	m_memoryMonitor = std::make_unique<RendererMemoryMonitor>(backendDiagnostics);
	m_frameExecutionDiagnostics.resize(RenderConfig::FramesInFlight);
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}
}

void Renderer::InitializeSceneSystems(LevelManager& levelManager) noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeSceneSystems");
	m_textureManager = std::make_unique<TextureManager>(GetRenderHardwareInterface());
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, GetRenderHardwareInterface());
	m_renderSceneDataBuilder = std::make_unique<RenderSceneDataBuilder>(*m_materialCacheManager, *m_gpuMeshCache);
	m_perViewDataBuilder = std::make_unique<PerViewDataBuilder>();
	m_viewLightingBuilder = std::make_unique<ViewLightingBuilder>();
	m_sceneSnapshot = std::make_unique<RenderSceneSnapshot>();

	m_renderCamera = std::make_unique<RenderCamera>();

	m_sceneRenderStateCoordinator = std::make_unique<SceneRenderStateCoordinator>(
	    levelManager.GetLevelChangeEvents(),
	    *m_gameScene,
	    *m_backend,
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_sceneSnapshot,
	    *m_renderCamera,
	    *m_materialCacheManager);
}

RenderViewportExtent Renderer::ResolveSceneExtent() const noexcept
{
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return RenderViewportExtent{static_cast<std::uint32_t>(m_window->GetWidth()), static_cast<std::uint32_t>(m_window->GetHeight())};
}

bool Renderer::ShouldPresentSceneToBackBuffer() const noexcept
{
	return m_viewportRenderRequest.ViewportId == 0;
}

void Renderer::InitializeFrameGraph() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeFrameGraph");
	const FrameGraphDependencies dependencies{
	    GetRenderHardwareInterface(),
	    *m_window,
	    ResolveSceneExtent(),
	    ShouldPresentSceneToBackBuffer()};

	FrameGraphFactory frameGraphFactory(dependencies);
	FrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphSceneExtent = dependencies.sceneExtent;

	m_viewportSceneColorHandle =
	    buildResult.SceneColor.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneColor.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_viewportSceneDepthHandle =
	    buildResult.SceneDepth.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneDepth.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_frameGraph = std::move(buildResult.Graph);
}

void Renderer::BindWindowResizeEvent() noexcept
{
	auto handle = m_window->OnResized.Add(
	    [this]()
	    {
		    m_bResizePending = true;
	    });
	m_resizeHandle = ScopedEventHandle(m_window->OnResized, handle);
}

void Renderer::RefreshFrameExecution() noexcept
{
	if (m_backend)
	{
		m_backend->Flush();
	}

	m_frameGraph.reset();
	InitializeFrameGraph();
}

void Renderer::BeginFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.BeginFrame");
	if (m_bResizePending)
	{
		m_bResizePending = false;

		if (m_window->HasValidSize())
		{
			m_backend->Flush();
			m_backend->ResizeSwapChain();
			RefreshFrameExecution();
		}
	}

	const RenderViewportExtent sceneExtent = ResolveSceneExtent();
	if (sceneExtent.Width != m_frameGraphSceneExtent.Width || sceneExtent.Height != m_frameGraphSceneExtent.Height)
	{
		RefreshFrameExecution();
	}

	m_backend->BeginFrame();
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
	ReportResolvedTimings(GetRenderHardwareInterface().GetCurrentFrameIndex(), frameDiagnostics);
}

void Renderer::SetupFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SetupFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame begin");

	m_timer->Tick();
	if (m_memoryMonitor != nullptr)
	{
		m_memoryMonitor->Tick(m_timer->GetFrameCount());
	}
	RefreshViewportRenderProducts();

	m_sceneSnapshot->Capture(m_gameScene->CaptureSnapshot());
	m_textureManager->LoadSceneTextures(m_sceneSnapshot->textures);
	m_renderCamera->Update(m_sceneSnapshot->camera);

	m_backend->UpdatePerFrameConstants(static_cast<std::uint32_t>(CVarRenderViewMode.Get()));
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame end");
}

void Renderer::RefreshViewportRenderProducts() noexcept
{
	const RenderViewportExtent extent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();

	m_viewportRenderProducts.Clear();
	m_viewportRenderProducts.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{m_viewportSceneColorHandle, extent, RenderProductFormat::ColorLdr});

	if (m_viewportSceneDepthHandle && HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{m_viewportSceneDepthHandle, extent, RenderProductFormat::DepthStencil});
	}
}

void Renderer::RecordFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RecordFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame build context begin");

	RenderHardwareInterface& renderHardwareInterface = GetRenderHardwareInterface();
	FrameContext frame = [&]()
	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.BuildFrameContext");
		return BuildFrameContext(
		    *m_sceneSnapshot,
		    renderHardwareInterface,
		    *m_renderCamera,
		    *m_renderSceneDataBuilder,
		    *m_perViewDataBuilder,
		    *m_viewLightingBuilder);
	}();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame build context end");

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
	const PassRuntimeServices passRuntimeServices{
	    .HardwareInterface = renderHardwareInterface,
	    .BackendDiagnostics = renderHardwareInterface.GetDiagnostics(),
	    .RuntimeManager = *m_pipelineStateManager,
	    .Textures = m_textureManager.get()};

	RenderCommandList& commandList = m_backend->GetCurrentGraphicsCommandList();
	RenderCommandContext cmd(commandList);
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	// A top-level GPU event covering the entire recorded frame.
	auto gpuFrameScope =
	    frameDiagnostics.BeginGpuEvent(cmd, "GPU Frame", RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255});
	auto gpuFrameTimer = frameDiagnostics.BeginTimer(cmd, "GPU Frame");

	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph execute begin");
	m_frameGraph->Execute(compiledPlan, cmd, frame, passRuntimeServices, frameDiagnostics);

	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph execute end");
}

void Renderer::SubmitFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SubmitFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SubmitFrame begin");
	m_backend->SubmitFrame();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SubmitFrame end");
}

void Renderer::EndFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.EndFrame");
	static const auto rendererLogger = Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::EndFrame begin");
	m_backend->AdvanceFrameInFlight();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::EndFrame end");
}

FrameExecutionDiagnostics& Renderer::GetCurrentFrameDiagnostics() noexcept
{
	return *m_frameExecutionDiagnostics[GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

const FrameExecutionDiagnostics& Renderer::GetCurrentFrameDiagnostics() const noexcept
{
	return *m_frameExecutionDiagnostics[GetRenderHardwareInterface().GetCurrentFrameIndex()];
}

void Renderer::ReportResolvedTimings(std::uint32_t frameIndex, const FrameExecutionDiagnostics& frameDiagnostics) const noexcept
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

void Renderer::PublishLiveGpuTimings(const std::vector<ResolvedGpuTiming>& resolvedTimers) const noexcept
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

void Renderer::PostLoad() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.PostLoad");
	m_backend->CloseExecuteAndFlushCurrentFrame();
}

void Renderer::OnRender() noexcept
{
	PrepareHostFrame();
	RecordHostFrame();
	SubmitHostFrame();
}

Renderer::~Renderer() noexcept
{
	if (m_backend)
	{
		m_backend->Flush();
	}
}
