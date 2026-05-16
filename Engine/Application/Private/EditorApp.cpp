#include "PCH.h"
#include "EditorApp.h"

#include "Editor/Public/UI.h"

#include "ProjectApp.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "ShaderRecook/ShaderConsoleCommands.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"

#include "Core/Public/Diagnostics/Trace.h"

EditorApp::EditorApp() = default;

EditorApp::~EditorApp() = default;

void EditorApp::Initialize()
{
	SPARKLE_CPU_SCOPE("Application.EditorInitialize");
	if (m_isEditorSessionActive)
	{
		return;
	}

	if (!m_projectApp)
	{
		m_projectApp = std::make_unique<ProjectApp>(ProjectAppOptions{.EnableRuntimeConsole = false});
	}

	if (!m_shaderRecookCoordinator)
	{
		m_shaderRecookCoordinator = std::make_unique<ShaderRecookCoordinator>();
	}

	m_projectApp->Initialize();
	m_projectApp->GetInputSystem().SetAutomaticImGuiCaptureEnabled(false);
	m_projectApp->GetInputSystem().BeginInputRoutingFrame(false, false);

	if (!m_ui)
	{
		Renderer& renderer = m_projectApp->GetRenderer();
		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();
		m_ui = std::make_unique<UI>(EditorHostServices{
		    .RuntimeTimer = m_projectApp->GetTimer(),
		    .Levels = m_projectApp->GetLevelManager(),
		    .Scene = m_projectApp->GetGameScene(),
		    .RenderHardware = renderHardware,
		    .HostWindow = m_projectApp->GetWindow(),
		    .Input = m_projectApp->GetInputSystem()});
		m_ui->SetDiagnosticsProviders(EditorDiagnosticsProviders{
		    .ShaderPackageGeneration = [&renderer]() noexcept
		    {
			    return renderer.GetShaderPackageGeneration();
		    },
		    .MeshDiagnostics = [&renderer]()
		    {
			    return renderer.CaptureMeshDiagnostics();
		    },
		    .TextureDiagnostics = [&renderer]()
		    {
			    return renderer.CaptureTextureDiagnostics();
		    },
		    .MemoryDiagnostics = [&renderer]()
		    {
			    return renderer.CaptureMemoryDiagnostics();
		    }});
		ShaderConsoleCommands::ConnectEditor(*m_ui, *m_shaderRecookCoordinator);
	}

	m_isEditorSessionActive = true;
}

bool EditorApp::Tick()
{
	SPARKLE_CPU_SCOPE("Application.CpuFrame");
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

	Renderer& renderer = m_projectApp->GetRenderer();
	if (m_shaderRecookCoordinator)
	{
		if (m_ui->ConsumeShaderRecookRequest())
		{
			m_shaderRecookCoordinator->RequestRecook();
		}

		m_shaderRecookCoordinator->Update(renderer, m_ui->ConsumeShaderReloadRequest());
	}

	m_projectApp->UpdateRuntime();
	m_projectApp->SubmitViewportRenderRequest(m_ui->GetViewportRenderRequest());

	renderer.PrepareHostFrame();
	renderer.RecordHostFrame();

	const ViewportRenderProducts& viewportProducts = m_projectApp->GetViewportRenderProducts();
	m_ui->SetViewportRenderProducts(viewportProducts);
	m_ui->SetViewportSceneColorTextureId(renderer.ResolveRenderProductTextureId(viewportProducts.GetSceneColor().Handle));
	m_ui->Update();

	RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();

	renderer.TransitionRenderProduct(
	    viewportProducts.GetSceneColor().Handle,
	    ResourceState::RenderTarget,
	    ResourceState::ShaderResource);

	constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	renderHardware.BeginPresentRenderPass(editorClearColor);
	m_ui->Render();

	renderHardware.EndPresentRenderPass();

	renderer.TransitionRenderProduct(
	    viewportProducts.GetSceneColor().Handle,
	    ResourceState::ShaderResource,
	    ResourceState::Common);

	renderer.SubmitHostFrame();
	m_projectApp->EndFrame();
	return true;
}

void EditorApp::Shutdown()
{
	SPARKLE_CPU_SCOPE("Application.EditorShutdown");
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_ui.reset();
	m_projectApp->GetInputSystem().SetAutomaticImGuiCaptureEnabled(true);
	m_projectApp->Shutdown();
	m_isEditorSessionActive = false;
}

void EditorApp::Run()
{
	Diagnostics::BeginTraceSession();

	Initialize();

	while (Tick())
	{
	}

	Shutdown();

	Diagnostics::EndTraceSession();
}