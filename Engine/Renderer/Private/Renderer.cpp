#include "PCH.h"
#include "Renderer.h"

#include "Runtime/Level/LevelManager.h"
#include "D3D12DebugLayer.h"
#include "D3D12Rhi.h"
#include "D3D12SwapChain.h"
#include "Window.h"
#include "TextureManager.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Scene/GameScene.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12FrameResource.h"
#include "Samplers/D3D12SamplerLibrary.h"
#include "UI.h"
#include "Time/Timer.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/CommandContext.h"
#include "Renderer/Public/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Scene/Camera/GameCamera.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "PipelineStateManager.h"
#include "SceneData/MaterialCacheManager.h"
#include "SceneData/SceneRenderStateCoordinator.h"
#include "SceneData/RenderSceneViewBuilder.h"

Renderer::Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept :
    m_timer(&timer), m_gameScene(&gameScene), m_window(&window)
{
	InitializeCoreSystems(levelManager);

	InitializeSceneSystems(levelManager);
	InitializeFrameGraph();
	BindWindowResizeEvent();

	PostLoad();
}

void Renderer::InitializeCoreSystems(LevelManager& levelManager) noexcept
{
	m_rhi = std::make_unique<D3D12Rhi>();

	m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*m_rhi);
	m_swapChain = std::make_unique<D3D12SwapChain>(*m_rhi, *m_window, *m_descriptorHeapManager);
	m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(*m_rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);
	m_pipelineStateManager = std::make_unique<PipelineStateManager>(*m_rhi);

	m_editor = std::make_unique<UI>(*m_timer, &levelManager, *m_rhi, *m_window, *m_descriptorHeapManager, *m_swapChain);

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
	m_renderSceneViewBuilder = std::make_unique<RenderSceneViewBuilder>(*m_materialCacheManager);

	m_renderCamera = std::make_unique<RenderCamera>(m_gameScene->GetCamera());
	m_sceneRenderStateCoordinator = std::make_unique<SceneRenderStateCoordinator>(
	    levelManager.GetLevelChangeEvents(),
	    *m_gameScene,
	    *m_rhi,
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_renderCamera,
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
	    *m_editor};

	FrameGraphBuilder frameGraphBuilder(dependencies);
	m_frameGraph = frameGraphBuilder.Build();
}

void Renderer::BindWindowResizeEvent() noexcept
{
	auto handle = m_window->OnResized.Add([this]() { m_bResizePending = true; });
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
	m_renderCamera->Update();

	m_timer->Tick();
	m_editor->Update();
	m_constantBufferManager->UpdatePerFrame();
}

void Renderer::RecordFrame() noexcept
{
	FrameContext frame = FrameContext::Build(*m_gameScene, *m_window, *m_swapChain, *m_renderCamera, *m_renderSceneViewBuilder);

	m_constantBufferManager->UpdatePerView(frame.perViewData);

	m_frameGraph->Setup(frame);
	const FrameGraph::CompiledPlan compiledPlan = m_frameGraph->Compile();

	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	CommandContext cmd(m_rhi->GetCommandList(frameIndex).Get());
	m_frameGraph->Execute(compiledPlan, cmd, frame);
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
