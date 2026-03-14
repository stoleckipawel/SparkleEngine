#pragma once

#include "Application/Public/ApplicationAPI.h"

#include <memory>

class Timer;
class Window;
class InputSystem;
class Scene;
class CameraController;
class Renderer;
class LevelManager;

class SPARKLE_APPLICATION_API App
{
  public:
	App();
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;
	App(App&&) = delete;
	App& operator=(App&&) = delete;

	void Run();

  private:
	void Initialize();
	void BeginFrame();
	void EndFrame();
	void EngineLoop();

	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<Scene> m_scene;
	std::unique_ptr<LevelManager> m_levelManager;
	std::unique_ptr<CameraController> m_cameraController;
	std::unique_ptr<Renderer> m_renderer;
};
