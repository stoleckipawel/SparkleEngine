#include "PCH.h"
#include "Renderer.h"

#include "Level/LevelManager.h"
#include "RHI/Public/Interop/RendererBackendServices.h"
#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "Window/Window.h"
#include "Textures/TextureManager.h"
#include "GPU/GPUMeshCache.h"
#include "Scene/GameScene.h"
#include "Time/Timer.h"
#include "Camera/RenderCamera.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Config/RenderConfig.h"
#include "GPU/CommandContext.h"
#include "GPU/FrameExecutionDiagnostics.h"
#include "Core/Public/Diagnostics/LiveProfiler.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "Frame/FrameContext.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassContext.h"
#include "Scene/Camera/CameraComponent.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Shadow/ShadowBuilder.h"
#include "Frame/Shadow/ShadowFrameBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Pipeline/PipelineStateManager.h"
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

	const ResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	return m_frameGraph->ResolveShaderResourceView(TextureHandle{resourceHandle}).Value;
}

void Renderer::TransitionRenderProduct(
    NativeGraphicsCommandListHandle commandList,
    RenderProductHandle handle,
    ResourceState before,
    ResourceState after) const noexcept
{
	if (!commandList || !handle || !m_frameGraph || before == after)
	{
		return;
	}

	const ResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	const NativeResourceHandle resource = m_frameGraph->ResolveResource(TextureHandle{resourceHandle});
	if (!resource)
	{
		return;
	}

	GetRenderHardwareInterface().TransitionResource(commandList, resource, before, after);
	m_frameGraph->UpdateTrackedResourceState(resourceHandle, after);
}

void Renderer::InitializeCoreSystems() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.InitializeCoreSystems");

	{
		SPARKLE_CPU_SCOPE("Renderer.CreateBackend");
		m_backend = RendererBackendServices::Create(*m_timer, *m_window);
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
	m_shadowBuilder = std::make_unique<ShadowBuilder>();
	m_shadowFrameBuilder = std::make_unique<ShadowFrameBuilder>();

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

	FrameGraphBuilder frameGraphBuilder(dependencies);
	FrameGraphBuildResult buildResult = frameGraphBuilder.Build();
	m_frameGraphSceneExtent = dependencies.sceneExtent;

	m_viewportRenderProducts.SceneColor.Handle =
	    buildResult.SceneColor.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneColor.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_viewportRenderProducts.SceneDepth.Handle =
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
	static const auto rendererLogger = Engine::Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame begin");

	m_timer->Tick();
	RefreshViewportRenderProducts();

	m_sceneSnapshot->Capture(*m_gameScene);
	m_textureManager->LoadSceneTextures(m_sceneSnapshot->textures);
	m_renderCamera->Update(m_sceneSnapshot->camera);

	m_backend->UpdatePerFrameConstants(static_cast<std::uint32_t>(CVarRenderViewMode.Get()));
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SetupFrame end");
}

void Renderer::RefreshViewportRenderProducts() noexcept
{
	const RenderProductHandle sceneColorHandle = m_viewportRenderProducts.SceneColor.Handle;
	const RenderProductHandle sceneDepthHandle = m_viewportRenderProducts.SceneDepth.Handle;
	const RenderViewportExtent extent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();

	m_viewportRenderProducts = {};
	m_viewportRenderProducts.AvailableOutputs = RenderOutputFlags::SceneColor;
	m_viewportRenderProducts.SceneColor.Handle = sceneColorHandle;
	m_viewportRenderProducts.SceneColor.Extent = extent;
	m_viewportRenderProducts.SceneColor.Format = RenderProductFormat::ColorLdr;

	if (sceneDepthHandle)
	{
		m_viewportRenderProducts.SceneDepth.Handle = sceneDepthHandle;
		m_viewportRenderProducts.SceneDepth.Extent = extent;
		m_viewportRenderProducts.SceneDepth.Format = RenderProductFormat::DepthStencil;

		if (HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
		{
			m_viewportRenderProducts.AvailableOutputs |= RenderOutputFlags::SceneDepth;
		}
	}
}

void Renderer::RecordFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.RecordFrame");
	static const auto rendererLogger = Engine::Logging::GetOrCreateLogger("Renderer");
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
		    *m_viewLightingBuilder,
		    *m_shadowFrameBuilder,
		    *m_shadowBuilder);
	}();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame build context end");

	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.FrameGraphSetup");
		m_frameGraph->Setup(frame);
	}
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph setup end");

	const FrameGraph::CompiledPlan compiledPlan = [&]()
	{
		SPARKLE_CPU_SCOPE("Renderer.RecordFrame.FrameGraphCompile");
		return m_frameGraph->Compile();
	}();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph compile end (passes={})", compiledPlan.executionOrder.size());
	const RenderPassContext renderPassContext{
	    .HardwareInterface = renderHardwareInterface,
	    .BackendDiagnostics = renderHardwareInterface.GetDiagnostics(),
	    .SamplerTableHandle = renderHardwareInterface.GetSamplerTableHandle(),
	    .RuntimeRegistry = m_pipelineStateManager->GetRuntimeRegistry()};

	RenderCommandList& commandList = m_backend->GetCurrentGraphicsCommandList();
	CommandContext cmd(commandList);
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	// A top-level GPU event covering the entire recorded frame.
	auto gpuFrameScope = frameDiagnostics.BeginGpuEvent(
	    cmd,
	    "GPU Frame",
	    RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255});
	auto gpuFrameTimer = frameDiagnostics.BeginTimer(cmd, "GPU Frame");

	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph execute begin");
	m_frameGraph->Execute(compiledPlan, cmd, frame, renderPassContext, frameDiagnostics);

	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::RecordFrame frame graph execute end");
}

void Renderer::SubmitFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.SubmitFrame");
	static const auto rendererLogger = Engine::Logging::GetOrCreateLogger("Renderer");
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SubmitFrame begin");
	m_backend->SubmitFrame();
	SPDLOG_LOGGER_TRACE(rendererLogger, "Renderer::SubmitFrame end");
}

void Renderer::EndFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Renderer.EndFrame");
	static const auto rendererLogger = Engine::Logging::GetOrCreateLogger("Renderer");
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

void Renderer::ReportResolvedTimings(
	std::uint32_t frameIndex,
	const FrameExecutionDiagnostics& frameDiagnostics) const noexcept
{
	const auto& resolvedTimers = frameDiagnostics.GetResolvedTimings();

	PublishLiveGpuTimings(resolvedTimers);

	static const auto rendererLogger = Engine::Logging::GetOrCreateLogger("Renderer");

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

	Engine::Diagnostics::LiveProfiler& profiler = Engine::Diagnostics::LiveProfiler::Get();
	if (!profiler.IsEnabled())
	{
		return;
	}

	std::vector<Engine::Diagnostics::LiveProfiler::GpuTimingEntry> entries;
	entries.reserve(resolvedTimers.size());
	for (const ResolvedGpuTiming& resolvedTimer : resolvedTimers)
	{
		entries.push_back(Engine::Diagnostics::LiveProfiler::GpuTimingEntry{
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
