#include "PCH.h"
#include "EditorApplication.h"

#include "Editor/Public/UI.h"

#include "RuntimeApplication.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "ShaderRecook/ShaderConsoleCommands.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"

#include "Core/Public/Diagnostics/Trace.h"

EditorApplication::EditorApplication() = default;

EditorApplication::~EditorApplication() = default;

void EditorApplication::Initialize()
{
	SPARKLE_CPU_SCOPE("Application.EditorInitialize");
	if (m_isEditorSessionActive)
	{
		return;
	}

	if (!m_runtimeApplication)
	{
		m_runtimeApplication = std::make_unique<RuntimeApplication>(RuntimeApplicationOptions{.EnableRuntimeConsole = false});
	}

	if (!m_shaderRecookCoordinator)
	{
		m_shaderRecookCoordinator = std::make_unique<ShaderRecookCoordinator>();
	}

	m_runtimeApplication->Initialize();
	m_runtimeApplication->GetInputSystem().SetAutomaticImGuiCaptureEnabled(false);
	m_runtimeApplication->GetInputSystem().BeginInputRoutingFrame(false, false);

	if (!m_ui)
	{
		Renderer& renderer = m_runtimeApplication->GetRenderer();
		m_ui = std::make_unique<UI>(EditorHostServices{
		    .RuntimeTimer = m_runtimeApplication->GetTimer(),
		    .Levels = m_runtimeApplication->GetLevelManager(),
		    .Scene = m_runtimeApplication->GetGameScene(),
		    .ImGuiRenderer = renderer.GetImGuiRenderer(),
		    .HostWindow = m_runtimeApplication->GetWindow(),
		    .Input = m_runtimeApplication->GetInputSystem()});
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

bool EditorApplication::Tick()
{
	SPARKLE_CPU_SCOPE("Application.CpuFrame");
	if (!m_isEditorSessionActive || !m_runtimeApplication || !m_ui)
	{
		return false;
	}

	switch (m_runtimeApplication->BeginFrame())
	{
		case RuntimeApplicationFrameResult::Exit:
			return false;
		case RuntimeApplicationFrameResult::SkipRender:
			return true;
		case RuntimeApplicationFrameResult::Ready:
		default:
			break;
	}

	Renderer& renderer = m_runtimeApplication->GetRenderer();
	if (m_shaderRecookCoordinator)
	{
		if (m_ui->ConsumeShaderRecookRequest())
		{
			m_shaderRecookCoordinator->RequestRecook();
		}

		m_shaderRecookCoordinator->Update(renderer, m_ui->ConsumeShaderReloadRequest());
	}

	m_runtimeApplication->UpdateRuntime();
	m_runtimeApplication->SubmitViewportRenderRequest(m_ui->GetViewportRenderRequest());

	renderer.PrepareHostFrame();
	renderer.RecordHostFrame();

	const ViewportRenderProducts& viewportProducts = m_runtimeApplication->GetViewportRenderProducts();
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
	m_runtimeApplication->EndFrame();
	return true;
}

void EditorApplication::Shutdown()
{
	SPARKLE_CPU_SCOPE("Application.EditorShutdown");
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_ui.reset();
	m_runtimeApplication->GetInputSystem().SetAutomaticImGuiCaptureEnabled(true);
	m_runtimeApplication->Shutdown();
	m_isEditorSessionActive = false;
}

