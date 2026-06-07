#include "PCH.h"
#include "RuntimeApplication.h"

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

RuntimeApplication::RuntimeApplication() = default;

RuntimeApplication::RuntimeApplication(RuntimeApplicationOptions options) noexcept : m_options(options) {}

RuntimeApplication::~RuntimeApplication() = default;

Timer& RuntimeApplication::GetTimer() noexcept
{
	return *m_timer;
}

Window& RuntimeApplication::GetWindow() noexcept
{
	return *m_window;
}

InputSystem& RuntimeApplication::GetInputSystem() noexcept
{
	return *m_inputSystem;
}

GameScene* RuntimeApplication::GetGameScene() const noexcept
{
	return m_gameScene.get();
}

LevelManager* RuntimeApplication::GetLevelManager() const noexcept
{
	return m_levelManager.get();
}

Renderer& RuntimeApplication::GetRenderer() noexcept
{
	return *m_renderer;
}

void RuntimeApplication::Initialize()
{
	SPARKLE_CPU_SCOPE("Application.RuntimeInitialize");
	if (m_isInitialized)
	{
		return;
	}

	{
		SPARKLE_CPU_SCOPE("Application.RuntimeCreateWindow");
		m_timer = std::make_unique<Timer>();
		m_window = std::make_unique<Window>("Sparkle Engine");
	}

	{
		SPARKLE_CPU_SCOPE("Application.RuntimeCreateInput");
		m_inputSystem = InputSystem::Create();
		m_inputSystem->SubscribeToWindow(*m_window);
	}

	{
		SPARKLE_CPU_SCOPE("Application.RuntimeCreateScene");
		m_gameScene = std::make_unique<GameScene>();
		m_sceneAssetManager = std::make_unique<Assets::SceneAssetManager>();
		m_levelManager = std::make_unique<LevelManager>(*m_gameScene, *m_sceneAssetManager);
		m_gameCameraController = std::make_unique<GameCameraController>(*m_timer, *m_inputSystem, *m_window, m_gameScene->GetCameras().GetActiveCamera());
	}

	{
		SPARKLE_CPU_SCOPE("Application.RuntimeCreateRenderer");
		m_renderer = std::make_unique<Renderer>(*m_timer, *m_gameScene, *m_window, *m_levelManager);
	}

	if (m_options.EnableRuntimeConsole)
	{
		SPARKLE_CPU_SCOPE("Application.RuntimeCreateConsole");
		m_runtimeConsoleHost = std::make_unique<RuntimeConsoleHost>(*m_timer, *m_window, *m_renderer);
	}
	m_isInitialized = true;
}

RuntimeApplicationFrameResult RuntimeApplication::BeginFrame()
{
	SPARKLE_CPU_SCOPE("Application.RuntimeBeginFrame");
	if (!m_isInitialized)
	{
		return RuntimeApplicationFrameResult::Exit;
	}

	m_inputSystem->BeginFrame();
	m_window->PollEvents();
	m_inputSystem->ProcessDeferredEvents();

	if (m_window->ShouldClose())
	{
		m_inputSystem->EndFrame();
		return RuntimeApplicationFrameResult::Exit;
	}

	if (m_window->IsMinimized())
	{
		m_inputSystem->EndFrame();
		m_window->WaitForEvent();
		return RuntimeApplicationFrameResult::SkipRender;
	}

	if (m_levelManager)
	{
		m_levelManager->ProcessPendingLevelChange();
	}

	return RuntimeApplicationFrameResult::Ready;
}

void RuntimeApplication::UpdateRuntime() noexcept
{
	SPARKLE_CPU_SCOPE("Application.RuntimeUpdate");
	if (m_gameScene && m_timer)
	{
		m_gameScene->Update(static_cast<float>(m_timer->GetDelta(TimeDomain::Scaled, TimeUnit::Seconds)));
	}
	if (m_gameCameraController)
	{
		m_gameCameraController->Update();
	}
}

void RuntimeApplication::SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
{
	if (m_renderer)
	{
		m_renderer->SubmitViewportRenderRequest(request);
	}
}

const ViewportRenderProducts& RuntimeApplication::GetViewportRenderProducts() const noexcept
{
	static const ViewportRenderProducts emptyProducts{};
	if (!m_renderer)
	{
		return emptyProducts;
	}

	return m_renderer->GetViewportRenderProducts();
}

void RuntimeApplication::EndFrame() noexcept
{
	SPARKLE_CPU_SCOPE("Application.RuntimeEndFrame");
	if (m_inputSystem)
	{
		m_inputSystem->EndFrame();
	}
}

bool RuntimeApplication::Tick()
{
	switch (BeginFrame())
	{
		case RuntimeApplicationFrameResult::Exit:
			return false;
		case RuntimeApplicationFrameResult::SkipRender:
			return true;
		case RuntimeApplicationFrameResult::Ready:
		default:
			break;
	}

	if (m_runtimeConsoleHost != nullptr)
	{
		m_runtimeConsoleHost->TickFrame(
		    *m_renderer,
		    [this]()
		    {
			    UpdateRuntime();
		    });
	}
	else
	{
		UpdateRuntime();
		m_renderer->OnRender();
	}
	EndFrame();
	return true;
}

void RuntimeApplication::Shutdown()
{
	SPARKLE_CPU_SCOPE("Application.RuntimeShutdown");
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

