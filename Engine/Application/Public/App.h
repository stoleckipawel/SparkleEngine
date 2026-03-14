#pragma once

#include "Application/Public/ApplicationAPI.h"

#include <memory>
#include <string>

class Timer;
class Window;
class InputSystem;
class Scene;
class CameraController;
class Renderer;
class AssetSystem;
class LevelRegistry;

class SPARKLE_APPLICATION_API App
{
  public:
	explicit App(std::string windowTitle);
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

	std::string m_windowTitle;
	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<LevelRegistry> m_levelRegistry;
	std::unique_ptr<AssetSystem> m_assetSystem;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<Scene> m_scene;
	std::unique_ptr<CameraController> m_cameraController;
	std::unique_ptr<Renderer> m_renderer;
};


