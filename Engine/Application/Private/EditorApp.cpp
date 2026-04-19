#include "PCH.h"
#include "EditorApp.h"

#include "Editor/Public/UI.h"

#include "ProjectApp.h"
#include "Renderer.h"

#include "Core/Public/Diagnostics/Trace.h"

EditorApp::EditorApp() = default;

EditorApp::~EditorApp() = default;

void EditorApp::Initialize()
{
	SPARKLE_CPU_SCOPE("Application.Editor.Initialize");
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
		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();
		m_ui = std::make_unique<UI>(
		    m_projectApp->GetTimer(),
		    m_projectApp->GetLevelManager(),
		    m_projectApp->GetGameScene(),
		    renderHardware,
		    m_projectApp->GetWindow());
	}

	m_isEditorSessionActive = true;
}

bool EditorApp::Tick()
{
	SPARKLE_CPU_SCOPE("Application.Editor.Tick");
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

	RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();

	const NativeGraphicsCommandListHandle commandListHandle =
	    renderHardware.GetGraphicsCommandListHandle(renderHardware.GetCurrentFrameIndex());

	renderer.TransitionRenderProduct(
	    commandListHandle,
	    viewportProducts.SceneColor.Handle,
	    ResourceState::RenderTarget,
	    ResourceState::ShaderResource);

	constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	renderHardware.BeginPresentRenderPass(commandListHandle, editorClearColor);
	m_ui->Render(commandListHandle);

	renderHardware.EndPresentRenderPass(commandListHandle);

	renderer.TransitionRenderProduct(
	    commandListHandle,
	    viewportProducts.SceneColor.Handle,
	    ResourceState::ShaderResource,
	    ResourceState::Common);

	renderer.SubmitHostFrame();
	m_projectApp->EndFrame();
	return true;
}

void EditorApp::Shutdown()
{
	SPARKLE_CPU_SCOPE("Application.Editor.Shutdown");
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
	Engine::Diagnostics::BeginTraceSession();

	Initialize();

	while (Tick())
	{
	}

	Shutdown();

	Engine::Diagnostics::EndTraceSession();
}