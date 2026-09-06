#include "PCH.h"
#include "EditorApplication.h"

#include "Editor/EditorUiFrameRenderer.h"
#include "Editor/Public/UI.h"
#include "Editor/Capture/EditorViewportCaptureCoordinator.h"
#include "EditorOperations/EditorOperationService.h"
#include "Input/InputSystem.h"
#include "Renderer.h"
#include "RuntimeApplication.h"
#include "ShaderRecook/ShaderConsoleCommands.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"
#include "Time/Timer.h"
#include "World/GameWorld.h"

#include <utility>

EditorApplication::EditorApplication() = default;

EditorApplication::EditorApplication(RuntimeApplicationOptions options) noexcept :
    m_runtimeOptions(options)
{
}

EditorApplication::~EditorApplication() = default;

void EditorApplication::Initialize()
{
	if (m_isEditorSessionActive)
	{
		return;
	}

	InitializeRuntimeApplication();
	InitializeEditorOperations();
	InitializeUi();
	m_isEditorSessionActive = true;
}

void EditorApplication::InitializeRuntimeApplication()
{
	if (!m_runtimeApplication)
	{
		RuntimeApplicationOptions runtimeOptions = m_runtimeOptions;
		runtimeOptions.EnableRuntimeConsole = false;
		runtimeOptions.EnableUiRenderPackets = true;
		m_runtimeApplication = std::make_unique<RuntimeApplication>(runtimeOptions);
	}

	m_runtimeApplication->Initialize();
}

void EditorApplication::InitializeEditorOperations()
{
	if (!m_operationService)
	{
		m_operationService = std::make_unique<EditorOperationService>(
		    m_runtimeApplication->GetTaskExecutor(),
		    m_runtimeApplication->GetApplicationTaskScope());
	}

	if (!m_shaderRecookCoordinator)
	{
		m_shaderRecookCoordinator = std::make_unique<ShaderRecookCoordinator>(*m_operationService);
	}

	if (!m_viewportCaptureCoordinator)
	{
		m_viewportCaptureCoordinator = std::make_unique<EditorViewportCaptureCoordinator>(*m_operationService);
	}
}

void EditorApplication::InitializeUi()
{
	m_runtimeApplication->GetInputSystem().ClearInputCaptureQuery();
	m_runtimeApplication->GetInputSystem().BeginInputRoutingFrame(false, false);
	if (m_ui)
	{
		return;
	}

	Renderer& renderer = m_runtimeApplication->GetRenderer();
	GameWorld& world = m_runtimeApplication->GetWorldForEditor();
	m_ui = std::make_unique<UI>(BuildUiHostServices(renderer, world));

	ConfigureUiDiagnostics(renderer);
	ShaderConsoleCommands::ConnectEditor(*m_ui, *m_shaderRecookCoordinator);
}

EditorHostServices EditorApplication::BuildUiHostServices(Renderer& renderer, GameWorld& world)
{
	return EditorHostServices{
	    .RuntimeTimer = m_runtimeApplication->GetTimer(),
	    .Levels = m_runtimeApplication->GetLevelSession(),
	    .AcquireWorldReadView = [&world]() { return world.AcquireReadView(); },
	    .ReadWorldChanges = [&world](const WorldChangeCursor& cursor) { return world.ReadChanges(cursor); },
	    .AcknowledgeWorldChanges = [&world](WorldChangeCursor& cursor, WorldSequence sequence)
	    { return world.AcknowledgeChanges(cursor, sequence); },
	    .WorldGeneration = [&world]() noexcept { return world.GetGeneration(); },
	    .MaterialVariants = [&world]() { return world.CaptureMaterialVariants(); },
	    .SubmitWorldEdit = [&world](WorldEditCommand command, std::uint64_t generation)
	    { return world.SubmitEdit(std::move(command), generation); },
	    .SubmitRenderingSettings = [&renderer](EngineRenderingSettingsState settings) { renderer.SubmitRenderingSettings(settings); },
	    .HostWindow = m_runtimeApplication->GetWindow(),
	    .Input = m_runtimeApplication->GetInputSystem()};
}

void EditorApplication::ConfigureUiDiagnostics(Renderer& renderer)
{
	m_ui->SetDiagnosticsProviders(
	    EditorDiagnosticsProviders{
	        .ShaderGeneration = [&renderer]() noexcept { return renderer.GetShaderGeneration(); },
	        .MeshDiagnostics = [&renderer]() { return renderer.CaptureMeshDiagnostics(); },
	        .TextureDiagnostics = [&renderer]() { return renderer.CaptureTextureDiagnostics(); },
	        .MemoryDiagnostics = [&renderer]() { return renderer.CaptureMemoryDiagnostics(); },
	        .MeshPreview = [&renderer](std::uintptr_t meshRuntimeId) { return renderer.CaptureMeshPreview(meshRuntimeId); }});
}

bool EditorApplication::Tick()
{
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
	UpdateEditorOperations(renderer);
	const ViewportRenderRequest& viewportRequest = m_ui->GetViewportRenderRequest();
	const float aspectRatio = viewportRequest.Extent.IsValid()
	    ? static_cast<float>(viewportRequest.Extent.Width) / static_cast<float>(viewportRequest.Extent.Height)
	    : 1.0f;
	const CameraInputIntent cameraIntent = m_runtimeApplication->CollectCameraInputIntent(aspectRatio);
	const float deltaSeconds = static_cast<float>(m_runtimeApplication->GetTimer().GetDelta(TimeDomain::Scaled, TimeUnit::Seconds));
	const RenderViewCameraData renderCamera = m_ui->UpdateViewportCamera(cameraIntent, deltaSeconds);
	m_runtimeApplication->UpdateEditorRuntime(renderCamera);
	RenderEditorFrame(renderer);
	return true;
}

void EditorApplication::UpdateEditorOperations(Renderer& renderer)
{
	if (m_viewportCaptureCoordinator)
	{
		m_viewportCaptureCoordinator->Update(renderer);
	}

	if (!m_shaderRecookCoordinator)
	{
		return;
	}

	if (m_ui->ConsumeShaderRecookRequest())
	{
		m_shaderRecookCoordinator->RequestRecook();
	}

	m_shaderRecookCoordinator->Update(renderer, m_ui->ConsumeShaderReloadRequest());
}

void EditorApplication::RenderEditorFrame(Renderer& renderer)
{
	EditorUiFrameRenderer::Render(*m_runtimeApplication, renderer, *m_ui);
	if (m_viewportCaptureCoordinator && m_ui->ConsumeViewportCaptureRequest())
	{
		m_viewportCaptureCoordinator->Request(renderer, m_runtimeApplication->GetTimer().GetFrameCount());
	}

	m_runtimeApplication->SubmitViewportRenderRequest(m_ui->GetViewportRenderRequest());
}

void EditorApplication::Shutdown()
{
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_ui.reset();
	m_viewportCaptureCoordinator.reset();
	m_shaderRecookCoordinator.reset();
	m_operationService.reset();
	m_runtimeApplication->Shutdown();
	m_isEditorSessionActive = false;
}
