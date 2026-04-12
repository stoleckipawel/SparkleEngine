#include "PCH.h"
#include "ProjectApp.h"

#include "Window/Window.h"
#include "Renderer.h"
#include "Assets/SceneAssetManager.h"
#include "Input/InputSystem.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/GameCameraController.h"
#include "Level/LevelManager.h"
#include "Time/Timer.h"

ProjectApp::ProjectApp() = default;

ProjectApp::~ProjectApp() = default;

void ProjectApp::Initialize()
{
	if (m_isInitialized)
	{
		return;
	}

	m_timer = std::make_unique<Timer>();

	m_window = std::make_unique<Window>("Sparkle Engine");

	m_inputSystem = InputSystem::Create();
	m_inputSystem->SubscribeToWindow(*m_window);

	m_gameScene = std::make_unique<GameScene>();
	m_sceneAssetManager = std::make_unique<Engine::Assets::SceneAssetManager>();
	m_levelManager = std::make_unique<LevelManager>(*m_gameScene, *m_sceneAssetManager);

	m_gameCameraController = std::make_unique<GameCameraController>(*m_timer, *m_inputSystem, *m_window, m_gameScene->GetSceneCamera());

	m_renderer = std::make_unique<Renderer>(*m_timer, *m_gameScene, *m_window, *m_levelManager);
	m_isInitialized = true;
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

bool ProjectApp::Tick()
{
	if (!m_isInitialized)
	{
		return false;
	}

	m_inputSystem->BeginFrame();
	m_window->PollEvents();
	m_inputSystem->ProcessDeferredEvents();

	if (m_window->ShouldClose())
	{
		m_inputSystem->EndFrame();
		return false;
	}

	if (m_window->IsMinimized())
	{
		m_inputSystem->EndFrame();
		m_window->WaitForEvent();
		return true;
	}

	m_gameCameraController->Update();
	m_renderer->OnRender();
	m_inputSystem->EndFrame();
	return true;
}

void ProjectApp::Shutdown()
{
	if (!m_isInitialized)
	{
		return;
	}

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
	Initialize();

	while (Tick())
	{
	}

	Shutdown();
}