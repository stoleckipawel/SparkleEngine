#include "PCH.h"
#include "Renderer.h"

#include "Level/LevelManager.h"
#include "D3D12/D3D12DebugLayer.h"
#include "D3D12/D3D12Rhi.h"
#include "D3D12/D3D12SwapChain.h"
#include "Window/Window.h"
#include "Renderer/Public/Textures/TextureManager.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Scene/GameScene.h"
#include "D3D12/Resources/D3D12ConstantBufferManager.h"
#include "D3D12/Resources/D3D12FrameResource.h"
#include "D3D12/Samplers/D3D12SamplerLibrary.h"
#include "Time/Timer.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Renderer/Public/GPU/CommandContext.h"
#include "Renderer/Public/Frame/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/FrameGraph/RenderPassContext.h"
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

Renderer::Renderer(
	Timer& timer,
	GameScene& gameScene,
	Window& window,
	LevelManager& levelManager) noexcept :
	m_timer(&timer), m_gameScene(&gameScene), m_window(&window)
{
	InitializeCoreSystems();

	InitializeSceneSystems(levelManager);
	InitializeFrameGraph();
	BindWindowResizeEvent();

	PostLoad();
}

void Renderer::InitializeCoreSystems() noexcept
{
	m_rhi = std::make_unique<D3D12Rhi>();

	m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*m_rhi);
	m_swapChain = std::make_unique<D3D12SwapChain>(*m_rhi, *m_window, *m_descriptorHeapManager);
	m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(*m_rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	m_pipelineStateManager = std::make_unique<PipelineStateManager>(*m_rhi);

	m_constantBufferManager = std::make_unique<D3D12ConstantBufferManager>(
	    *m_timer,
	    *m_rhi,
	    *m_window,
	    *m_descriptorHeapManager,
	    *m_frameResourceManager,
	    *m_swapChain);

	m_samplerLibrary = std::make_unique<D3D12SamplerLibrary>(*m_rhi, *m_descriptorHeapManager);
	m_gpuMeshCache = std::make_unique<GPUMeshCache>(*m_rhi);
}

void Renderer::InitializeSceneSystems(LevelManager& levelManager) noexcept
{
	m_textureManager = std::make_unique<TextureManager>(*m_rhi, *m_descriptorHeapManager);
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, *m_descriptorHeapManager);
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
	    *m_rhi,
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_sceneSnapshot,
	    *m_renderCamera,
	    *m_materialCacheManager);
}

void Renderer::InitializeFrameGraph() noexcept
{
	const FrameGraphDependencies dependencies{*m_rhi, *m_window, *m_swapChain, *m_descriptorHeapManager};

	FrameGraphBuilder frameGraphBuilder(dependencies);
	FrameGraphBuildResult buildResult = frameGraphBuilder.Build();

	m_viewportRenderProducts.SceneColor.Handle = buildResult.SceneColor.IsValid()
	    ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneColor.GetResourceHandle().index) + 1ull}
	    : RenderProductHandle{};
	m_viewportRenderProducts.SceneDepth.Handle = buildResult.SceneDepth.IsValid()
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
	m_frameGraph.reset();
	InitializeFrameGraph();
}

void Renderer::BeginFrame() noexcept
{
	if (m_bResizePending)
	{
		m_bResizePending = false;

		if (m_window->HasValidSize())
		{
			m_rhi->Flush();
			m_swapChain->Resize();
			RefreshFrameExecution();
		}
	}

	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	m_rhi->SetCurrentFrameIndex(frameIndex);
	m_frameResourceManager->BeginFrame(m_rhi->GetFence().Get(), m_rhi->GetFenceEvent(), frameIndex);
	m_rhi->WaitForGPU(frameIndex);
	m_rhi->ResetCommandAllocator(frameIndex);
	m_rhi->ResetCommandList(frameIndex);
}

void Renderer::SetupFrame() noexcept
{
	m_timer->Tick();
	RefreshViewportRenderProducts();

	m_sceneSnapshot->Capture(*m_gameScene);
	m_textureManager->LoadSceneTextures(m_sceneSnapshot->textures);
	m_renderCamera->Update(m_sceneSnapshot->camera);

	m_constantBufferManager->UpdatePerFrame(static_cast<std::uint32_t>(CVarRenderViewMode.Get()));
}

void Renderer::RefreshViewportRenderProducts() noexcept
{
	const RenderProductHandle sceneColorHandle = m_viewportRenderProducts.SceneColor.Handle;
	const RenderProductHandle sceneDepthHandle = m_viewportRenderProducts.SceneDepth.Handle;
	const RenderViewportExtent extent{
	    static_cast<std::uint32_t>(m_window->GetWidth()),
	    static_cast<std::uint32_t>(m_window->GetHeight())};

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
	FrameContext frame = BuildFrameContext(
	    *m_sceneSnapshot,
	    *m_swapChain,
	    *m_constantBufferManager,
	    *m_renderCamera,
	    *m_renderSceneDataBuilder,
	    *m_perViewDataBuilder,
	    *m_viewLightingBuilder,
	    *m_shadowFrameBuilder,
	    *m_shadowBuilder);

	m_frameGraph->Setup(frame);
	const FrameGraph::CompiledPlan compiledPlan = m_frameGraph->Compile();
	const RenderPassContext renderPassContext{
	    .DescriptorHeapManager = *m_descriptorHeapManager,
	    .ConstantBufferManager = *m_constantBufferManager,
	    .SamplerLibrary = *m_samplerLibrary,
	    .RuntimeRegistry = m_pipelineStateManager->GetRuntimeRegistry()};

	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	CommandContext cmd(m_rhi->GetCommandList(frameIndex).Get());
	m_frameGraph->Execute(compiledPlan, cmd, frame, renderPassContext);
}

void Renderer::SubmitFrame() noexcept
{
	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	m_rhi->CloseCommandList(frameIndex);
	m_rhi->ExecuteCommandList(frameIndex);
	m_rhi->Signal(frameIndex);

	m_frameResourceManager->EndFrame(m_rhi->GetNextFenceValue() - 1);
	m_swapChain->Present();
}

void Renderer::EndFrame() noexcept
{
	m_swapChain->UpdateFrameInFlightIndex();
}

void Renderer::PostLoad() noexcept
{
	const uint32_t frameIndex = m_rhi->GetCurrentFrameIndex();
	m_rhi->CloseCommandList(frameIndex);
	m_rhi->ExecuteCommandList(frameIndex);
	m_rhi->Flush();
}

void Renderer::OnRender() noexcept
{
	BeginFrame();
	SetupFrame();
	RecordFrame();
	SubmitFrame();
	EndFrame();
}

Renderer::~Renderer() noexcept
{
	if (m_rhi)
	{
		m_rhi->Flush();
	}
}
