#include "PCH.h"
#include "App.h"

#include "Window.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "Scene/Scene.h"
#include "Scene/Camera/CameraController.h"
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

	m_scene = std::make_unique<Scene>();
	m_levelManager = std::make_unique<LevelManager>(*m_scene);

	m_cameraController = std::make_unique<CameraController>(*m_timer, *m_inputSystem, *m_window, m_scene->GetCamera());

	m_renderer = std::make_unique<Renderer>(*m_timer, *m_scene, *m_window, *m_levelManager);
}

void App::BeginFrame()
{
	m_inputSystem->BeginFrame();
	m_window->PollEvents();
	m_inputSystem->ProcessDeferredEvents();
	m_cameraController->Update();
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