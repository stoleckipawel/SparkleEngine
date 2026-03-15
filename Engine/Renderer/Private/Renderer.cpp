#include "PCH.h"
#include "Renderer.h"

#include "Runtime/Level/LevelManager.h"
#include "D3D12DebugLayer.h"
#include "D3D12Rhi.h"
#include "D3D12SwapChain.h"
#include "Window.h"
#include "TextureManager.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Scene/Scene.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12FrameResource.h"
#include "Samplers/D3D12SamplerLibrary.h"
#include "UI.h"
#include "Time/Timer.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Scene/Camera/GameCamera.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Frame/FrameExecutor.h"
#include "PipelineStateManager.h"
#include "RendererWindowObserver.h"
#include "SceneData/MaterialCacheManager.h"
#include "SceneData/RenderSceneSnapshotCache.h"
#include "SceneData/SceneRenderStateCoordinator.h"
#include "SceneData/SceneViewBuilder.h"

Renderer::Renderer(Timer& timer, Scene& scene, Window& window, LevelManager& levelManager) noexcept :
    m_timer(&timer), m_scene(&scene), m_window(&window)
{
	InitializeCoreSystems(levelManager);

	InitializeSceneSystems(levelManager);
	InitializeFrameGraph();
	InitializeFrameExecutor();
	InitializeWindowObserver();

	PostLoad();
}

void Renderer::InitializeCoreSystems(LevelManager& levelManager) noexcept
{
	m_rhi = std::make_unique<D3D12Rhi>();

	m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*m_rhi);
	m_swapChain = std::make_unique<D3D12SwapChain>(*m_rhi, *m_window, *m_descriptorHeapManager);
	m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(*m_rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	m_pipelineStateManager = std::make_unique<PipelineStateManager>(*m_rhi);

	m_ui = std::make_unique<UI>(*m_timer, &levelManager, *m_rhi, *m_window, *m_descriptorHeapManager, *m_swapChain);

	m_constantBufferManager = std::make_unique<D3D12ConstantBufferManager>(
	    *m_timer,
	    *m_rhi,
	    *m_window,
	    *m_descriptorHeapManager,
	    *m_frameResourceManager,
	    *m_swapChain,
	    *m_ui);

	m_samplerLibrary = std::make_unique<D3D12SamplerLibrary>(*m_rhi, *m_descriptorHeapManager);
	m_gpuMeshCache = std::make_unique<GPUMeshCache>(*m_rhi);
}

void Renderer::InitializeSceneSystems(LevelManager& levelManager) noexcept
{
	m_renderSceneSnapshotCache = std::make_unique<RenderSceneSnapshotCache>();
	m_textureManager = std::make_unique<TextureManager>(*m_rhi, *m_descriptorHeapManager);
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, *m_descriptorHeapManager);
	m_sceneViewBuilder = std::make_unique<SceneViewBuilder>(*m_materialCacheManager);

	m_renderCamera = std::make_unique<RenderCamera>(m_scene->GetCamera());
	m_sceneRenderStateCoordinator = std::make_unique<SceneRenderStateCoordinator>(
	    levelManager.GetLevelChangeEvents(),
	    *m_scene,
	    *m_rhi,
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_renderCamera,
	    *m_renderSceneSnapshotCache,
	    *m_materialCacheManager);
}

void Renderer::InitializeFrameGraph() noexcept
{
	const FrameGraphDependencies dependencies{
	    *m_rhi,
	    *m_window,
	    m_pipelineStateManager->GetRootSignature(),
	    m_pipelineStateManager->GetPipelineState(),
	    *m_constantBufferManager,
	    *m_textureManager,
	    *m_samplerLibrary,
	    *m_gpuMeshCache,
	    *m_swapChain,
	    *m_descriptorHeapManager,
	    *m_ui};

	FrameGraphBuilder frameGraphBuilder(dependencies);
	m_frameGraph = frameGraphBuilder.Build();
}

void Renderer::InitializeFrameExecutor() noexcept
{
	m_frameExecutor = std::make_unique<FrameExecutor>(
	    *m_timer,
	    *m_scene,
	    *m_window,
	    *m_rhi,
	    *m_swapChain,
	    *m_frameResourceManager,
	    *m_renderCamera,
	    *m_ui,
	    *m_constantBufferManager,
	    *m_frameGraph,
	    *m_renderSceneSnapshotCache,
	    *m_sceneViewBuilder);
}

void Renderer::InitializeWindowObserver() noexcept
{
	m_windowObserver = std::make_unique<RendererWindowObserver>(
	    *m_window,
	    [this]()
	    {
		    m_rhi->Flush();
		    m_swapChain->Resize();
		    RefreshFrameExecution();
	    });
}

void Renderer::RefreshFrameExecution() noexcept
{
	m_frameExecutor.reset();
	m_frameGraph.reset();
	InitializeFrameGraph();
	InitializeFrameExecutor();
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
	m_frameExecutor->ExecuteFrame();
}

Renderer::~Renderer() noexcept
{
	if (m_rhi)
	{
		m_rhi->Flush();
	}
}
