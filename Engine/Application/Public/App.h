#pragma once

#include "Application/Public/ApplicationAPI.h"

#include <memory>

class Timer;
class Window;
class InputSystem;
class GameScene;
class GameCameraController;
class Renderer;
class LevelManager;
namespace Engine::Assets
{
	class SceneAssetManager;
}

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
	void EngineLoop();

	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<GameScene> m_gameScene;
	std::unique_ptr<Engine::Assets::SceneAssetManager> m_sceneAssetManager;
	std::unique_ptr<LevelManager> m_levelManager;
	std::unique_ptr<GameCameraController> m_gameCameraController;
	std::unique_ptr<Renderer> m_renderer;
};
