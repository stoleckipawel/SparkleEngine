#pragma once

#include "Application/Public/ApplicationAPI.h"
#include "Renderer/Public/Overlays/RendererOverlay.h"

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

class SPARKLE_APPLICATION_API ProjectApp
{
  public:
	explicit ProjectApp(RendererOverlayFactory overlayFactory = {});
	~ProjectApp();

	ProjectApp(const ProjectApp&) = delete;
	ProjectApp& operator=(const ProjectApp&) = delete;
	ProjectApp(ProjectApp&&) = delete;
	ProjectApp& operator=(ProjectApp&&) = delete;

	void Initialize();
	bool Tick();
	void Shutdown();
	void Run();

  private:
	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<GameScene> m_gameScene;
	std::unique_ptr<Engine::Assets::SceneAssetManager> m_sceneAssetManager;
	std::unique_ptr<LevelManager> m_levelManager;
	std::unique_ptr<GameCameraController> m_gameCameraController;
	std::unique_ptr<Renderer> m_renderer;
	RendererOverlayFactory m_overlayFactory;
	bool m_isInitialized = false;
};