#include "PCH.h"
#include "App.h"

#include "Window.h"
#include "Renderer.h"
#include "Input/InputSystem.h"
#include "Scene/Scene.h"
#include "Scene/Camera/CameraController.h"
#include "Assets/AssetSystem.h"
#include "Level/Level.h"
#include "Level/LevelRegistry.h"
#include "Time/Timer.h"
#include "Core/Public/Diagnostics/Log.h"

#include <utility>

App::App(std::string windowTitle) : m_windowTitle(windowTitle.empty() ? "Sparkle" : std::move(windowTitle))
{
}

App::~App() = default;

void App::Initialize()
{
	m_timer = std::make_unique<Timer>();

	m_levelRegistry = std::make_unique<LevelRegistry>();

	m_assetSystem = std::make_unique<AssetSystem>();

	m_window = std::make_unique<Window>(m_windowTitle);

	m_inputSystem = InputSystem::Create();
	m_inputSystem->SubscribeToWindow(*m_window);

	m_scene = std::make_unique<Scene>();
	m_scene->LoadLevelOrDefault(*m_levelRegistry, m_windowTitle, *m_assetSystem);

	m_cameraController = std::make_unique<CameraController>(*m_timer, *m_inputSystem, *m_window, m_scene->GetCamera());

	m_renderer = std::make_unique<Renderer>(*m_timer, *m_assetSystem, *m_scene, *m_window);
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