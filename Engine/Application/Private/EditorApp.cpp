#include "PCH.h"
#include "EditorApp.h"

#include "Editor/Public/UI.h"

#include "ProjectApp.h"
#include "Renderer.h"
#include "ShaderRecook/ShaderConsoleCommands.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/Console/ConsoleOutput.h"
#include "Core/Public/Diagnostics/Trace.h"

#include <utility>

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
		m_projectApp = std::make_unique<ProjectApp>(ProjectAppOptions{.EnableRuntimeConsoleOverlay = false});
	}

	if (!m_shaderRecookCoordinator)
	{
		m_shaderRecookCoordinator = std::make_unique<ShaderRecookCoordinator>();
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
		m_ui->SetShaderPackageGenerationProvider(
		    [&renderer]() noexcept
		    {
			    return renderer.GetShaderPackageGeneration();
		    });
		m_shaderRecookCoordinator->SetStatusHandler(
		    [this](std::string status)
		    {
			    if (m_ui)
			    {
				    const ConsoleCommandSeverity severity = status.find("failed") != std::string::npos ? ConsoleCommandSeverity::Error : ConsoleCommandSeverity::Info;
				    m_ui->AppendConsoleOutput(ConsoleOutputRecord{.Severity = severity, .Text = status});
				    m_ui->SetShaderRecookStatus(std::move(status));
			    }
		    });

		if (ConsoleCommandRegistry* consoleCommandRegistry = m_ui->GetConsoleCommandRegistry())
		{
			ShaderConsoleCommands::Register(
			    *consoleCommandRegistry,
			    ShaderConsoleCommands::Handlers{
			        .RequestRecook = [this](ShaderRecookRequest request)
			        {
				        if (m_shaderRecookCoordinator)
				        {
					        m_shaderRecookCoordinator->RequestRecook(std::move(request));
				        }
			        },
			        .RequestReload = [this]()
			        {
				        if (m_shaderRecookCoordinator)
				        {
					        m_shaderRecookCoordinator->RequestReload();
				        }
			        },
			    });
		}
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
	SPARKLE_CPU_SCOPE("Application.EditorShutdown");
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