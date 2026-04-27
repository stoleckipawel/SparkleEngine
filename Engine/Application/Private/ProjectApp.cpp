#include "PCH.h"
#include "ProjectApp.h"

#include "Window/Window.h"
#include "Renderer.h"
#include "RuntimeConsole/RuntimeConsoleHost.h"
#include "Assets/SceneAssetManager.h"
#include "Input/InputSystem.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/GameCameraController.h"
#include "Level/LevelManager.h"
#include "Time/Timer.h"

#include "Core/Public/Diagnostics/Trace.h"

ProjectApp::ProjectApp() = default;

ProjectApp::ProjectApp(ProjectAppOptions options) noexcept : m_options(options) {}

ProjectApp::~ProjectApp() = default;

Timer& ProjectApp::GetTimer() noexcept
{
	return *m_timer;
}

Window& ProjectApp::GetWindow() noexcept
{
	return *m_window;
}

InputSystem& ProjectApp::GetInputSystem() noexcept
{
	return *m_inputSystem;
}

GameScene* ProjectApp::GetGameScene() const noexcept
{
	return m_gameScene.get();
}

LevelManager* ProjectApp::GetLevelManager() const noexcept
{
	return m_levelManager.get();
}

Renderer& ProjectApp::GetRenderer() noexcept
{
	return *m_renderer;
}

void ProjectApp::Initialize()
{
	SPARKLE_CPU_SCOPE("Application.ProjectInitialize");
	if (m_isInitialized)
	{
		return;
	}

	{
		SPARKLE_CPU_SCOPE("Application.ProjectCreateWindow");
		m_timer = std::make_unique<Timer>();
		m_window = std::make_unique<Window>("Sparkle Engine");
	}

	{
		SPARKLE_CPU_SCOPE("Application.ProjectCreateInput");
		m_inputSystem = InputSystem::Create();
		m_inputSystem->SubscribeToWindow(*m_window);
	}

	{
		SPARKLE_CPU_SCOPE("Application.ProjectCreateScene");
		m_gameScene = std::make_unique<GameScene>();
		m_sceneAssetManager = std::make_unique<Assets::SceneAssetManager>();
		m_levelManager = std::make_unique<LevelManager>(*m_gameScene, *m_sceneAssetManager);
		m_gameCameraController = std::make_unique<GameCameraController>(*m_timer, *m_inputSystem, *m_window, m_gameScene->GetSceneCamera());
	}

	{
		SPARKLE_CPU_SCOPE("Application.ProjectCreateRenderer");
		m_renderer = std::make_unique<Renderer>(*m_timer, *m_gameScene, *m_window, *m_levelManager);
	}

	if (m_options.EnableRuntimeConsole)
	{
		SPARKLE_CPU_SCOPE("Application.ProjectCreateRuntimeConsole");
		m_runtimeConsoleHost = std::make_unique<RuntimeConsoleHost>(*m_timer, *m_window, *m_renderer);
	}
	m_isInitialized = true;
}

ProjectAppFrameResult ProjectApp::BeginFrame()
{
	SPARKLE_CPU_SCOPE("Application.ProjectBeginFrame");
	if (!m_isInitialized)
	{
		return ProjectAppFrameResult::Exit;
	}

	m_inputSystem->BeginFrame();
	m_window->PollEvents();
	m_inputSystem->ProcessDeferredEvents();

	if (m_window->ShouldClose())
	{
		m_inputSystem->EndFrame();
		return ProjectAppFrameResult::Exit;
	}

	if (m_window->IsMinimized())
	{
		m_inputSystem->EndFrame();
		m_window->WaitForEvent();
		return ProjectAppFrameResult::SkipRender;
	}

	return ProjectAppFrameResult::Ready;
}

void ProjectApp::UpdateRuntime() noexcept
{
	SPARKLE_CPU_SCOPE("Application.ProjectUpdateRuntime");
	if (m_gameCameraController)
	{
		m_gameCameraController->Update();
	}
}

void ProjectApp::SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
{
	if (m_renderer)
	{
		m_renderer->SubmitViewportRenderRequest(request);
	}
}

const ViewportRenderProducts& ProjectApp::GetViewportRenderProducts() const noexcept
{
	static const ViewportRenderProducts emptyProducts{};
	if (!m_renderer)
	{
		return emptyProducts;
	}

	return m_renderer->GetViewportRenderProducts();
}

void ProjectApp::EndFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Application.ProjectEndFrame");
	if (m_inputSystem)
	{
		m_inputSystem->EndFrame();
	}
}

bool ProjectApp::Tick()
{
	switch (BeginFrame())
	{
		case ProjectAppFrameResult::Exit:
			return false;
		case ProjectAppFrameResult::SkipRender:
			return true;
		case ProjectAppFrameResult::Ready:
		default:
			break;
	}

	if (m_runtimeConsoleHost != nullptr)
	{
		m_runtimeConsoleHost->TickFrame(*m_renderer, [this]() { UpdateRuntime(); });
	}
	else
	{
		UpdateRuntime();
		m_renderer->OnRender();
	}
	EndFrame();
	return true;
}

void ProjectApp::Shutdown()
{
	SPARKLE_CPU_SCOPE("Application.ProjectShutdown");
	if (!m_isInitialized)
	{
		return;
	}

	m_runtimeConsoleHost.reset();
	m_renderer.reset();
	m_gameCameraController.reset();
	m_levelManager.reset();
	m_sceneAssetManager.reset();
	m_gameScene.reset();
	m_inputSystem.reset();
	m_window.reset();
	m_timer.reset();
	m_isInitialized = false;
}

void ProjectApp::Run()
{
	Diagnostics::BeginTraceSession();

	Initialize();

	while (Tick())
	{
	}

	Shutdown();

	Diagnostics::EndTraceSession();
}