#include "PCH.h"
#include "FrameExecutor.h"

#include "D3D12FrameResource.h"
#include "D3D12Rhi.h"
#include "D3D12SwapChain.h"
#include "D3D12ConstantBufferManager.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/CommandContext.h"
#include "Renderer/Public/FrameContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Scene/Scene.h"
#include "SceneData/RenderSceneSnapshotCache.h"
#include "SceneData/SceneViewBuilder.h"
#include "Time/Timer.h"
#include "UI.h"
#include "Window.h"

FrameExecutor::FrameExecutor(
    Timer& timer,
    Scene& scene,
    Window& window,
    D3D12Rhi& rhi,
    D3D12SwapChain& swapChain,
    D3D12FrameResourceManager& frameResourceManager,
    RenderCamera& renderCamera,
    UI& ui,
    D3D12ConstantBufferManager& constantBufferManager,
    FrameGraph& frameGraph,
    RenderSceneSnapshotCache& renderSceneSnapshotCache,
    SceneViewBuilder& sceneViewBuilder) noexcept :
    m_timer(&timer),
    m_scene(&scene),
    m_window(&window),
    m_rhi(&rhi),
    m_swapChain(&swapChain),
    m_frameResourceManager(&frameResourceManager),
    m_renderCamera(&renderCamera),
    m_ui(&ui),
    m_constantBufferManager(&constantBufferManager),
    m_frameGraph(&frameGraph),
    m_renderSceneSnapshotCache(&renderSceneSnapshotCache),
    m_sceneViewBuilder(&sceneViewBuilder)
{
}

void FrameExecutor::ExecuteFrame() noexcept
{
	BeginFrame();
	SetupFrame();
	RecordFrame();
	SubmitFrame();
	EndFrame();
}

void FrameExecutor::BeginFrame() noexcept
{
	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	m_rhi->SetCurrentFrameIndex(frameIndex);
	m_frameResourceManager->BeginFrame(m_rhi->GetFence().Get(), m_rhi->GetFenceEvent(), frameIndex);
	m_rhi->WaitForGPU(frameIndex);
	m_rhi->ResetCommandAllocator(frameIndex);
	m_rhi->ResetCommandList(frameIndex);
}

void FrameExecutor::SetupFrame() noexcept
{
	m_renderCamera->Update();

	m_timer->Tick();
	m_ui->Update();
	m_constantBufferManager->UpdatePerFrame();
}

void FrameExecutor::RecordFrame() noexcept
{
	FrameContext frame =
	    FrameContext::Build(*m_scene, *m_window, *m_swapChain, *m_renderCamera, *m_renderSceneSnapshotCache, *m_sceneViewBuilder);

	m_constantBufferManager->UpdatePerView(frame.perViewData);

	m_frameGraph->Setup(frame);
	const FrameGraph::CompiledPlan compiledPlan = m_frameGraph->Compile();

	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	CommandContext cmd(m_rhi->GetCommandList(frameIndex).Get());
	m_frameGraph->Execute(compiledPlan, cmd, frame);
}

void FrameExecutor::SubmitFrame() noexcept
{
	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	m_rhi->CloseCommandList(frameIndex);
	m_rhi->ExecuteCommandList(frameIndex);
	m_rhi->Signal(frameIndex);

	m_frameResourceManager->EndFrame(m_rhi->GetNextFenceValue() - 1);
	m_swapChain->Present();
}

void FrameExecutor::EndFrame() noexcept
{
	m_swapChain->UpdateFrameInFlightIndex();
}