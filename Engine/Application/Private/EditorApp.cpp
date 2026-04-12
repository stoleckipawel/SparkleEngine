#include "PCH.h"
#include "EditorApp.h"

#include "Editor/Public/UI.h"

#include "ProjectApp.h"
#include "Renderer.h"

#include "RHI/Public/D3D12/D3D12Rhi.h"
#include "RHI/Public/D3D12/D3D12SwapChain.h"
#include "RHI/Public/D3D12/Descriptors/D3D12DescriptorHeapManager.h"
#include "Renderer/Public/GPU/CommandContext.h"

EditorApp::EditorApp() = default;

EditorApp::~EditorApp() = default;

void EditorApp::Initialize()
{
	if (m_isEditorSessionActive)
	{
		return;
	}

	if (!m_projectApp)
	{
		m_projectApp = std::make_unique<ProjectApp>();
	}

	m_projectApp->Initialize();
	if (!m_ui)
	{
		Renderer& renderer = m_projectApp->GetRenderer();
		m_ui = std::make_unique<UI>(
		    m_projectApp->GetTimer(),
		    m_projectApp->GetLevelManager(),
		    m_projectApp->GetGameScene(),
		    renderer.GetRhi(),
		    m_projectApp->GetWindow(),
		    renderer.GetDescriptorHeapManager(),
		    renderer.GetSwapChain());
	}
	m_isEditorSessionActive = true;
}

bool EditorApp::Tick()
{
	if (!m_isEditorSessionActive || !m_projectApp || !m_ui)
	{
		return false;
	}

	switch (m_projectApp->BeginFrame())
	{
		case ProjectAppFrameResult::Exit:
			return false;
		case ProjectAppFrameResult::SkipRender:
			return true;
		case ProjectAppFrameResult::Ready:
		default:
			break;
	}

	m_projectApp->UpdateRuntime();
	m_projectApp->SubmitViewportRenderRequest(m_ui->GetViewportRenderRequest());

	Renderer& renderer = m_projectApp->GetRenderer();
	renderer.PrepareHostFrame();
	renderer.RecordHostFrame();

	const ViewportRenderProducts& viewportProducts = m_projectApp->GetViewportRenderProducts();
	m_ui->SetViewportRenderProducts(viewportProducts);
	m_ui->SetViewportSceneColorTextureId(renderer.ResolveRenderProductTextureId(viewportProducts.SceneColor.Handle));
	m_ui->Update();

	ID3D12GraphicsCommandList* commandList = renderer.GetRhi().GetCommandList(renderer.GetRhi().GetCurrentFrameIndex()).Get();
	CommandContext uiCommandContext(commandList);
	renderer.TransitionRenderProduct(
	    uiCommandContext,
	    viewportProducts.SceneColor.Handle,
	    ResourceState::RenderTarget,
	    ResourceState::ShaderResource);
	uiCommandContext.TransitionResource(renderer.GetSwapChain().GetCurrentResource(), ResourceState::Present, ResourceState::RenderTarget);
	renderer.GetDescriptorHeapManager().SetShaderVisibleHeaps(uiCommandContext);
	uiCommandContext.SetRenderTarget(renderer.GetSwapChain().GetCPUHandle());
	constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	uiCommandContext.ClearRenderTarget(renderer.GetSwapChain().GetCPUHandle(), editorClearColor);
	m_ui->Render(commandList);
	uiCommandContext.TransitionResource(renderer.GetSwapChain().GetCurrentResource(), ResourceState::RenderTarget, ResourceState::Present);
	renderer.TransitionRenderProduct(
	    uiCommandContext,
	    viewportProducts.SceneColor.Handle,
	    ResourceState::ShaderResource,
	    ResourceState::Common);

	renderer.SubmitHostFrame();
	m_projectApp->EndFrame();
	return true;
}

void EditorApp::Shutdown()
{
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_ui.reset();
	m_projectApp->Shutdown();
	m_isEditorSessionActive = false;
}

void EditorApp::Run()
{
	Initialize();

	while (Tick())
	{
	}

	Shutdown();
}