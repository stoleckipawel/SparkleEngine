#include "PCH.h"
#include "EditorApplication.h"

#include "Editor/EditorUiFrameRenderer.h"
#include "Editor/Public/UI.h"

#include "RuntimeApplication.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "ShaderRecook/ShaderConsoleCommands.h"
#include "ShaderRecook/ShaderRecookCoordinator.h"
#include "EditorOperations/EditorOperationService.h"
#include "World/GameWorld.h"

EditorApplication::EditorApplication() = default;

EditorApplication::EditorApplication(RuntimeApplicationOptions options) noexcept : m_runtimeOptions(std::move(options)) {}

EditorApplication::~EditorApplication() = default;

void EditorApplication::Initialize()
{
	if (m_isEditorSessionActive)
	{
		return;
	}

	if (!m_runtimeApplication)
	{
		RuntimeApplicationOptions runtimeOptions = m_runtimeOptions;
		runtimeOptions.EnableRuntimeConsole = false;
		runtimeOptions.AllowThreadedRenderer = false;
		m_runtimeApplication = std::make_unique<RuntimeApplication>(std::move(runtimeOptions));
	}

	m_runtimeApplication->Initialize();
	if (!m_operationService)
	{
		m_operationService = std::make_unique<EditorOperationService>(
		    m_runtimeApplication->GetTaskExecutor(), m_runtimeApplication->GetApplicationTaskScope());
	}
	if (!m_shaderRecookCoordinator)
		m_shaderRecookCoordinator = std::make_unique<ShaderRecookCoordinator>(*m_operationService);
	m_runtimeApplication->GetInputSystem().ClearInputCaptureQuery();
	m_runtimeApplication->GetInputSystem().BeginInputRoutingFrame(false, false);

	if (!m_ui)
	{
		Renderer& renderer = m_runtimeApplication->GetRenderer();
		GameWorld& world = *m_runtimeApplication->GetGameWorld();
		m_ui = std::make_unique<UI>(EditorHostServices{
		    .RuntimeTimer = m_runtimeApplication->GetTimer(),
		    .Levels = m_runtimeApplication->GetLevelManager(),
		    .AcquireWorldReadView = [&world]() { return world.AcquireReadView(); },
		    .ReadWorldChanges = [&world](const WorldChangeCursor& cursor) { return world.ReadChanges(cursor); },
		    .AcknowledgeWorldChanges = [&world](WorldChangeCursor& cursor, WorldSequence sequence) {
			    return world.AcknowledgeChanges(cursor, sequence);
		    },
		    .WorldGeneration = [&world]() noexcept { return world.GetGeneration(); },
		    .MaterialVariants = [&world]() { return world.CaptureMaterialVariants(); },
		    .SubmitWorldEdit = [&world](WorldEditCommand command, std::uint64_t generation) {
			    return world.SubmitEdit(std::move(command), generation);
		    },
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
		    },
		    .MeshPreview = [&renderer](std::uintptr_t meshRuntimeId) { return renderer.CaptureMeshPreview(meshRuntimeId); }});
		ShaderConsoleCommands::ConnectEditor(*m_ui, *m_shaderRecookCoordinator);
	}

	m_isEditorSessionActive = true;
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

	EditorUiFrameRenderer::Render(*m_runtimeApplication, renderer, *m_ui);
	return true;
}

void EditorApplication::Shutdown()
{
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_ui.reset();
	m_shaderRecookCoordinator.reset();
	m_operationService.reset();
	m_runtimeApplication->Shutdown();
	m_isEditorSessionActive = false;
}
