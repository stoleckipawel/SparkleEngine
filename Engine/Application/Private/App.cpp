#include "PCH.h"
#include "App.h"

#include "Window.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "Scene/GameScene.h"
#include "Scene/Camera/GameCameraController.h"
#include "Runtime/Level/LevelManager.h"
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

	m_gameCameraController = std::make_unique<GameCameraController>(*m_timer, *m_inputSystem, *m_window, m_gameScene->GetCamera());
	m_levelManager->RegisterGameCameraController(*m_gameCameraController);

	m_renderer = std::make_unique<Renderer>(*m_timer, *m_gameScene, *m_window, *m_levelManager);
}

void App::BeginFrame()
{
	m_inputSystem->BeginFrame();
	m_window->PollEvents();
	m_inputSystem->ProcessDeferredEvents();
	m_gameCameraController->Update();
}

void App::EndFrame()
{
	m_inputSystem->EndFrame();
}

void App::EngineLoop()
{
	while (!m_window->ShouldClose())
	{
		BeginFrame();
		m_renderer->OnRender();
		EndFrame();
	}
}

void App::Run()
{
	Initialize();
	EngineLoop();
}