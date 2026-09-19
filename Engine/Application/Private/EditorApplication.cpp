#include "PCH.h"
#include "EditorApplication.h"

#include "Editor/EditorUiFrameRenderer.h"
#include "Editor/Public/UI.h"
#include "Editor/Viewport/EditorViewportOutputCoordinator.h"
#include "EditorOperations/EditorOperationRuntime.h"
#include "Input/InputSystem.h"
#include "Renderer.h"
#include "RuntimeApplication.h"
#include "ShaderRecook/ShaderConsoleCommands.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"
#include "Time/Timer.h"
#include "World/GameWorld.h"

#include <utility>

struct EditorApplication::State final
{
	std::unique_ptr<RuntimeApplication> Runtime;
	std::unique_ptr<UI> Ui;
	std::unique_ptr<EditorOperationRuntime> OperationRuntime;
	std::unique_ptr<ShaderRecookCoordinator> ShaderRecook;
	std::unique_ptr<EditorViewportOutputCoordinator> ViewportOutput;
};

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
	if (!m_state)
	{
		m_state = std::make_unique<State>();
	}

	InitializeRuntimeApplication();
	InitializeEditorOperations();
	InitializeUi();
	m_isEditorSessionActive = true;
}

void EditorApplication::InitializeRuntimeApplication()
{
	if (!m_state->Runtime)
	{
		RuntimeApplicationOptions runtimeOptions = m_runtimeOptions;
		runtimeOptions.EnableRuntimeConsole = false;
		runtimeOptions.EnableUiRenderPackets = true;
		m_state->Runtime = std::make_unique<RuntimeApplication>(runtimeOptions);
	}

	m_state->Runtime->Initialize();
}

void EditorApplication::InitializeEditorOperations()
{
	if (!m_state->OperationRuntime)
	{
		m_state->OperationRuntime =
		    std::make_unique<EditorOperationRuntime>(m_state->Runtime->GetTaskExecutor(), m_state->Runtime->GetApplicationTaskScope());
	}

	if (!m_state->ShaderRecook)
	{
		m_state->ShaderRecook = std::make_unique<ShaderRecookCoordinator>(*m_state->OperationRuntime);
	}

	if (!m_state->ViewportOutput)
	{
		m_state->ViewportOutput = std::make_unique<EditorViewportOutputCoordinator>(*m_state->OperationRuntime);
	}
}

void EditorApplication::InitializeUi()
{
	m_state->Runtime->GetInputSystem().ClearInputCaptureQuery();
	m_state->Runtime->GetInputSystem().BeginInputRoutingFrame(false, false);
	if (m_state->Ui)
	{
		return;
	}

	Renderer& renderer = m_state->Runtime->GetRenderer();
	GameWorld& world = m_state->Runtime->GetWorldForEditor();
	m_state->Ui = std::make_unique<UI>(EditorHostServices{
	    .RuntimeTimer = m_state->Runtime->GetTimer(),
	    .Levels = m_state->Runtime->GetLevelSession(),
	    .AcquireWorldReadView = [&world]() { return world.AcquireReadView(); },
	    .ReadWorldChanges = [&world](const WorldChangeCursor& cursor) { return world.ReadChanges(cursor); },
	    .AcknowledgeWorldChanges = [&world](WorldChangeCursor& cursor, WorldSequence sequence)
	    { return world.AcknowledgeChanges(cursor, sequence); },
	    .WorldGeneration = [&world]() noexcept { return world.GetGeneration(); },
	    .MaterialVariants = [&world]() { return world.CaptureMaterialVariants(); },
	    .SubmitWorldEdit = [&world](WorldEditCommand command, std::uint64_t generation)
	    { return world.SubmitEdit(std::move(command), generation); },
	    .RenderingSettings = renderer.CaptureRenderingSettings(),
	    .SubmitRenderingSettings = [&renderer](EngineRenderingSettingsState settings)
	    {
		    SaveRenderingSettings(settings);
		    renderer.SubmitRenderingSettings(std::move(settings));
	    },
	    .CaptureRenderingSettings = [&renderer]() { return renderer.CaptureRenderingSettings(); },
	    .HostWindow = m_state->Runtime->GetWindow(),
	    .Input = m_state->Runtime->GetInputSystem()});

	ConfigureUiDiagnostics(renderer);
	ShaderConsoleCommands::ConnectEditor(*m_state->Ui, *m_state->ShaderRecook);
}

void EditorApplication::ConfigureUiDiagnostics(Renderer& renderer)
{
	m_state->Ui->SetDiagnosticsProviders(
	    EditorDiagnosticsProviders{
	        .ShaderGeneration = [&renderer]() noexcept { return renderer.GetShaderGeneration(); },
	        .MeshDiagnostics = [&renderer]() { return renderer.CaptureMeshDiagnostics(); },
	        .TextureDiagnostics = [&renderer]() { return renderer.CaptureTextureDiagnostics(); },
	        .MemoryDiagnostics = [&renderer]() { return renderer.CaptureMemoryDiagnostics(); },
	        .MeshPreview = [&renderer](std::uintptr_t meshRuntimeId) { return renderer.CaptureMeshPreview(meshRuntimeId); }});
}

bool EditorApplication::Tick()
{
	if (!m_isEditorSessionActive || !m_state->Runtime || !m_state->Ui)
	{
		return false;
	}

	switch (m_state->Runtime->BeginFrame())
	{
		case RuntimeApplicationFrameResult::Exit:
			return false;
		case RuntimeApplicationFrameResult::SkipRender:
			return true;
		case RuntimeApplicationFrameResult::Ready:
		default:
			break;
	}

	Renderer& renderer = m_state->Runtime->GetRenderer();
	UpdateEditorOperations(renderer);
	const ViewportRenderRequest& viewportRequest = m_state->Ui->GetViewportRenderRequest();
	const float aspectRatio = viewportRequest.Extent.IsValid()
	    ? static_cast<float>(viewportRequest.Extent.Width) / static_cast<float>(viewportRequest.Extent.Height)
	    : 1.0f;
	const CameraInputIntent cameraIntent = m_state->Runtime->CollectCameraInputIntent(aspectRatio);
	const float deltaSeconds = static_cast<float>(m_state->Runtime->GetTimer().GetDelta(TimeDomain::Scaled, TimeUnit::Seconds));
	const RenderViewCameraData renderCamera = m_state->Ui->UpdateViewportCamera(cameraIntent, deltaSeconds);
	m_state->Runtime->UpdateEditorRuntime(renderCamera);
	RenderEditorFrame(renderer);
	return true;
}

void EditorApplication::UpdateEditorOperations(Renderer& renderer)
{
	m_state->ViewportOutput->Update(renderer);

	if (m_state->Ui->ConsumeShaderRecookRequest())
	{
		m_state->ShaderRecook->RequestRecook();
	}

	m_state->ShaderRecook->Update(renderer, m_state->Ui->ConsumeShaderReloadRequest());
}

void EditorApplication::RenderEditorFrame(Renderer& renderer)
{
	EditorUiFrameRenderer::Render(*m_state->Runtime, renderer, *m_state->Ui);
	m_state->ViewportOutput->HandleAction(*m_state->Ui, renderer, m_state->Runtime->GetTimer().GetFrameCount());

	m_state->Runtime->SubmitViewportRenderRequest(m_state->Ui->GetViewportRenderRequest());
}

void EditorApplication::Shutdown()
{
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_state->Ui.reset();
	m_state->ViewportOutput.reset();
	m_state->ShaderRecook.reset();
	m_state->OperationRuntime.reset();
	m_state->Runtime->Shutdown();
	m_isEditorSessionActive = false;
}
