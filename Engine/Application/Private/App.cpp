#include "PCH.h"
#include "App.h"

#include "Window.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/GameCameraController.h"
#include "Level/LevelManager.h"
#include "Time/Timer.h"

App::App() = default;

App::~App() = default;

void App::Initialize()
{
	m_timer = std::make_unique<Timer>();

	m_window = std::make_unique<Window>("Sparkle Engine");

	m_inputSystem = InputSystem::Create();
	m_inputSystem->SubscribeToWindow(*m_window);

	m_gameScene = std::make_unique<GameScene>();
	m_levelManager = std::make_unique<LevelManager>(*m_gameScene);

	m_gameCameraController = std::make_unique<GameCameraController>(*m_timer, *m_inputSystem, *m_window, m_gameScene->GetSceneCamera());

	m_renderer = std::make_unique<Renderer>(*m_timer, *m_gameScene, *m_window, *m_levelManager);
}

void App::EngineLoop()
{
	while (true)
	{
		m_inputSystem->BeginFrame();
		m_window->PollEvents();
		m_inputSystem->ProcessDeferredEvents();

		if (m_window->ShouldClose())
		{
			m_inputSystem->EndFrame();
			return;
		}

		if (m_window->IsMinimized())
		{
			m_inputSystem->EndFrame();
			m_window->WaitForEvent();
			continue;
		}

		m_gameCameraController->Update();
		m_renderer->OnRender();
		m_inputSystem->EndFrame();
	}
}

void App::Run()
{
	Initialize();
	EngineLoop();
}