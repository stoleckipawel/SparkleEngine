#pragma once

#include "ApplicationAPI.h"
#include "../../Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>

class Timer;
class Window;
class InputSystem;
class GameScene;
class GameCameraController;
class Renderer;
class LevelManager;
class RuntimeConsoleHost;
namespace Assets
{
	class SceneAssetManager;
}

enum class ProjectAppFrameResult : std::uint8_t
{
	Exit = 0,
	SkipRender = 1,
	Ready = 2,
};

struct ProjectAppOptions final
{
	bool EnableRuntimeConsole = true;
};

class SPARKLE_APPLICATION_API ProjectApp
{
  public:
	ProjectApp();
	explicit ProjectApp(ProjectAppOptions options) noexcept;
	~ProjectApp();

	ProjectApp(const ProjectApp&) = delete;
	ProjectApp& operator=(const ProjectApp&) = delete;
	ProjectApp(ProjectApp&&) = delete;
	ProjectApp& operator=(ProjectApp&&) = delete;

	void Initialize();
	ProjectAppFrameResult BeginFrame();
	void UpdateRuntime() noexcept;
	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept;
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept;
	void EndFrame() noexcept;
	Timer& GetTimer() noexcept;
	Window& GetWindow() noexcept;
	GameScene* GetGameScene() const noexcept;
	LevelManager* GetLevelManager() const noexcept;
	Renderer& GetRenderer() noexcept;
	bool Tick();
	void Shutdown();
	void Run();

  private:
	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<GameScene> m_gameScene;
	std::unique_ptr<Assets::SceneAssetManager> m_sceneAssetManager;
	std::unique_ptr<LevelManager> m_levelManager;
	std::unique_ptr<GameCameraController> m_gameCameraController;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<RuntimeConsoleHost> m_runtimeConsoleHost;
	ProjectAppOptions m_options;
	bool m_isInitialized = false;
};